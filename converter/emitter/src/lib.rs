//! Lowers GameRecomp PE32 IR to the ReHoMM4 web ABI and reports unsupported instructions.

#[cfg(all(feature = "browser", target_arch = "wasm32"))]
mod browser;
#[cfg(feature = "discovery")]
pub mod discovery;

use std::borrow::Cow;
use std::collections::{BTreeMap, BTreeSet};

use rc_ir::{
    BinOp, Expr, FArith, FBinary, FConst, FLoc, FUnary, Flag, Fpu, Function, Part, Reg, Stmt, Term,
    Tmp, UnOp, W,
};
use wasm_encoder::{
    BlockType, EntityType, ExportKind, ExportSection, FunctionSection, ImportSection,
    Instruction, MemArg, MemArgTarget, MemoryType, Module, TypeSection, ValType,
};
use wasm_encoder::Encode;

const GUEST_BASE: u32 = 0;
const DISPATCH: u32 = 1;
const FNINIT: u32 = 2;
const FLD32: u32 = 3;
const FST32: u32 = 4;
const FSTSW: u32 = 5;
const FPOP: u32 = 6;
const DIVIDE_ERROR: u32 = 7;
const SIGNED_DIVIDE: u32 = 8;
const FSTCW: u32 = 9;
const FLDCW: u32 = 10;
const X87_MEMORY: u32 = 11;
const X87_REGISTER: u32 = 12;
const X87_MISC: u32 = 13;
const INTERRUPT: u32 = 14;
const RESUME: u32 = 15;
const DISPATCH_OWNED: u32 = 16;
const HOST_FUNCTIONS: usize = 17;
const CONTEXT: u32 = 0;
const SHIFT_VALUE: u32 = 1;
const SHIFT_COUNT: u32 = 2;
// The guest memory base, read once per call (the runtime picks it when the game starts).
const BASE: u32 = 3;
// EAX..EDI and the seven flags live in locals inside a function (the engine drops the flag
// values nobody reads) and go back to the Ctx around calls, returns and runtime exits.
const REG0: u32 = 4;
const FLAG0: u32 = REG0 + 8;
const FIRST_TEMP: u32 = FLAG0 + 7;
const FLAGS: [Flag; 7] = [Flag::Cf, Flag::Pf, Flag::Af, Flag::Zf, Flag::Sf, Flag::Df, Flag::Of];
const FLAG_MASK: i32 = 0x0CD5;
const SAVED_EAX_OFFSET: u64 = 0;
const EFLAGS_OFFSET: u64 = 128;
const FS_BASE_OFFSET: u64 = 132;

fn flag_bit(flag: Flag) -> i32 {
    match flag {
        Flag::Cf => 0,
        Flag::Pf => 2,
        Flag::Af => 4,
        Flag::Zf => 6,
        Flag::Sf => 7,
        Flag::Df => 10,
        Flag::Of => 11,
    }
}

fn flag_local(flag: Flag) -> u32 {
    FLAG0 + FLAGS.iter().position(|f| *f == flag).unwrap() as u32
}

/// The registers and flags from the locals to the Ctx.
fn sync_out(code: &mut wasm_encoder::Function) {
    for n in 0..8u32 {
        code.instruction(&Instruction::LocalGet(CONTEXT));
        code.instruction(&Instruction::LocalGet(REG0 + n));
        code.instruction(&Instruction::I32Store(ctx_slot(SAVED_EAX_OFFSET + 8 * u64::from(n))));
    }
    code.instruction(&Instruction::LocalGet(CONTEXT));
    code.instruction(&Instruction::LocalGet(CONTEXT));
    code.instruction(&Instruction::I32Load(ctx_slot(EFLAGS_OFFSET)));
    code.instruction(&Instruction::I32Const(!FLAG_MASK));
    code.instruction(&Instruction::I32And);
    for flag in FLAGS {
        code.instruction(&Instruction::LocalGet(flag_local(flag)));
        code.instruction(&Instruction::I32Const(flag_bit(flag)));
        code.instruction(&Instruction::I32Shl);
        code.instruction(&Instruction::I32Or);
    }
    code.instruction(&Instruction::I32Store(ctx_slot(EFLAGS_OFFSET)));
}

/// The registers and flags from the Ctx to the locals (after the runtime or a callee ran).
fn sync_in(code: &mut wasm_encoder::Function) {
    for n in 0..8u32 {
        code.instruction(&Instruction::LocalGet(CONTEXT));
        code.instruction(&Instruction::I32Load(ctx_slot(SAVED_EAX_OFFSET + 8 * u64::from(n))));
        code.instruction(&Instruction::LocalSet(REG0 + n));
    }
    for flag in FLAGS {
        code.instruction(&Instruction::LocalGet(CONTEXT));
        code.instruction(&Instruction::I32Load(ctx_slot(EFLAGS_OFFSET)));
        code.instruction(&Instruction::I32Const(flag_bit(flag)));
        code.instruction(&Instruction::I32ShrU);
        code.instruction(&Instruction::I32Const(1));
        code.instruction(&Instruction::I32And);
        code.instruction(&Instruction::LocalSet(flag_local(flag)));
    }
}

fn reg_local(reg: &Reg) -> Result<u32, &'static str> {
    if reg.n >= 8 {
        return Err("invalid register");
    }
    Ok(REG0 + u32::from(reg.n))
}

fn ctx_slot(offset: u64) -> MemArg {
    MemArg {
        offset,
        align: 2,
        target: MemArgTarget::Memory(0),
        ordering: None,
    }
}


fn load(width: W, offset: u64) -> Result<Instruction<'static>, &'static str> {
    let slot = ctx_slot(offset);
    match width {
        W::B8 => Ok(Instruction::I32Load8U(MemArg { align: 0, ..slot })),
        W::B16 => Ok(Instruction::I32Load16U(MemArg { align: 1, ..slot })),
        W::B32 => Ok(Instruction::I32Load(slot)),
        W::B64 => Err("64-bit load is unsupported"),
    }
}

fn store(width: W, offset: u64) -> Result<Instruction<'static>, &'static str> {
    let slot = ctx_slot(offset);
    match width {
        W::B8 => Ok(Instruction::I32Store8(MemArg { align: 0, ..slot })),
        W::B16 => Ok(Instruction::I32Store16(MemArg { align: 1, ..slot })),
        W::B32 => Ok(Instruction::I32Store(slot)),
        W::B64 => Err("64-bit store is unsupported"),
    }
}

fn expression(
    code: &mut wasm_encoder::Function,
    value: &Expr,
    temporaries: &BTreeMap<Tmp, u32>,
) -> Result<(), &'static str> {
    let instruction = match value {
        Expr::Const(width @ (W::B8 | W::B16 | W::B32), value) => {
            Instruction::I32Const((value & width.mask()) as i32)
        }
        Expr::Const(W::B64, value) => Instruction::I64Const(*value as i64),
        Expr::Tmp(_, tmp) => Instruction::LocalGet(
            *temporaries
                .get(tmp)
                .ok_or("temporary read before assignment")?,
        ),
        Expr::Reg(reg) => {
            let local = reg_local(reg)?;
            match reg.part {
                Part::D => Instruction::LocalGet(local),
                Part::X | Part::L => {
                    code.instruction(&Instruction::LocalGet(local));
                    code.instruction(&Instruction::I32Const(if reg.part == Part::X { 0xFFFF } else { 0xFF }));
                    Instruction::I32And
                }
                Part::H => {
                    code.instruction(&Instruction::LocalGet(local));
                    code.instruction(&Instruction::I32Const(8));
                    code.instruction(&Instruction::I32ShrU);
                    code.instruction(&Instruction::I32Const(0xFF));
                    Instruction::I32And
                }
            }
        }
        Expr::Flag(flag) => Instruction::LocalGet(flag_local(*flag)),
        Expr::Trunc(width @ (W::B8 | W::B16 | W::B32), value) => {
            expression(code, value, temporaries)?;
            if value.width() == W::B64 {
                code.instruction(&Instruction::I32WrapI64);
            }
            code.instruction(&Instruction::I32Const(width.mask() as i32));
            Instruction::I32And
        }
        Expr::Un(UnOp::Parity, W::B8, value) => {
            expression(code, value, temporaries)?;
            code.instruction(&Instruction::I32Const(255));
            code.instruction(&Instruction::I32And);
            code.instruction(&Instruction::I32Popcnt);
            code.instruction(&Instruction::I32Const(1));
            code.instruction(&Instruction::I32And);
            Instruction::I32Eqz
        }
        Expr::FsBase => {
            code.instruction(&Instruction::LocalGet(CONTEXT));
            Instruction::I32Load(ctx_slot(FS_BASE_OFFSET))
        }
        Expr::FpuStatus => {
            code.instruction(&Instruction::LocalGet(CONTEXT));
            Instruction::Call(FSTSW)
        }
        Expr::FpuControl => {
            code.instruction(&Instruction::LocalGet(CONTEXT));
            Instruction::Call(FSTCW)
        }
        Expr::Zext(W::B16 | W::B32, value) if value.width() != W::B64 => {
            return expression(code, value, temporaries);
        }
        Expr::Zext(W::B64, value) => {
            expression(code, value, temporaries)?;
            Instruction::I64ExtendI32U
        }
        Expr::Sext(from @ (W::B8 | W::B16 | W::B32), W::B64, value) => {
            expression(code, value, temporaries)?;
            sign_extend_i32(code, *from);
            Instruction::I64ExtendI32S
        }
        Expr::Select(W::B8 | W::B16 | W::B32, condition, yes, no) => {
            expression(code, condition, temporaries)?;
            code.instruction(&Instruction::If(BlockType::Result(ValType::I32)));
            expression(code, yes, temporaries)?;
            code.instruction(&Instruction::Else);
            expression(code, no, temporaries)?;
            Instruction::End
        }
        Expr::Sext(from @ (W::B8 | W::B16), to @ (W::B16 | W::B32), value) => {
            expression(code, value, temporaries)?;
            sign_extend_i32(code, *from);
            code.instruction(&Instruction::I32Const(to.mask() as i32));
            Instruction::I32And
        }
        Expr::Un(UnOp::Not | UnOp::Neg, width @ (W::B8 | W::B16 | W::B32), operand) => {
            if matches!(value, Expr::Un(UnOp::Neg, ..)) {
                code.instruction(&Instruction::I32Const(0));
                expression(code, operand, temporaries)?;
                code.instruction(&Instruction::I32Sub);
            } else {
                expression(code, operand, temporaries)?;
                code.instruction(&Instruction::I32Const(-1));
                code.instruction(&Instruction::I32Xor);
            }
            code.instruction(&Instruction::I32Const(width.mask() as i32));
            Instruction::I32And
        }
        Expr::Bin(op, W::B64, left, right) => {
            expression(code, left, temporaries)?;
            expression(code, right, temporaries)?;
            if matches!(op, BinOp::Shl | BinOp::Shr | BinOp::Sar) {
                shift_i64(code, *op);
                return Ok(());
            }
            match op {
                BinOp::Add => Instruction::I64Add,
                BinOp::Sub => Instruction::I64Sub,
                BinOp::Mul => Instruction::I64Mul,
                BinOp::UDiv => Instruction::I64DivU,
                BinOp::URem => Instruction::I64RemU,
                BinOp::SRem => Instruction::I64RemS,
                BinOp::And => Instruction::I64And,
                BinOp::Or => Instruction::I64Or,
                BinOp::Xor => Instruction::I64Xor,
                BinOp::Eq => Instruction::I64Eq,
                BinOp::Ne => Instruction::I64Ne,
                BinOp::Ult => Instruction::I64LtU,
                BinOp::Ule => Instruction::I64LeU,
                BinOp::Slt => Instruction::I64LtS,
                BinOp::Sle => Instruction::I64LeS,
                _ => return Err("unsupported 64-bit arithmetic operation"),
            }
        }
        Expr::Bin(op, width @ (W::B8 | W::B16 | W::B32), left, right) => {
            expression(code, left, temporaries)?;
            if matches!(
                op,
                BinOp::Slt | BinOp::Sle | BinOp::Sar | BinOp::SDiv | BinOp::SRem
            ) {
                sign_extend_i32(code, *width);
            }
            if matches!(op, BinOp::Shl | BinOp::Shr | BinOp::Sar) {
                code.instruction(&if *op == BinOp::Sar {
                    Instruction::I64ExtendI32S
                } else {
                    Instruction::I64ExtendI32U
                });
                expression(code, right, temporaries)?;
                code.instruction(&Instruction::I64ExtendI32U);
                shift_i64(code, *op);
                code.instruction(&Instruction::I32WrapI64);
                code.instruction(&Instruction::I32Const(width.mask() as i32));
                code.instruction(&Instruction::I32And);
                return Ok(());
            }
            expression(code, right, temporaries)?;
            if matches!(op, BinOp::Slt | BinOp::Sle | BinOp::SDiv | BinOp::SRem) {
                sign_extend_i32(code, *width);
            }
            match op {
                BinOp::Add => Instruction::I32Add,
                BinOp::Sub => Instruction::I32Sub,
                BinOp::Mul => Instruction::I32Mul,
                BinOp::UDiv => Instruction::I32DivU,
                BinOp::URem => Instruction::I32RemU,
                BinOp::SDiv => Instruction::I32DivS,
                BinOp::SRem => Instruction::I32RemS,
                BinOp::And => Instruction::I32And,
                BinOp::Or => Instruction::I32Or,
                BinOp::Xor => Instruction::I32Xor,
                BinOp::Eq => Instruction::I32Eq,
                BinOp::Ne => Instruction::I32Ne,
                BinOp::Ult => Instruction::I32LtU,
                BinOp::Ule => Instruction::I32LeU,
                BinOp::Slt => Instruction::I32LtS,
                BinOp::Sle => Instruction::I32LeS,
                _ => return Err("unsupported arithmetic operation"),
            }
        }
        Expr::Load(width, address) => {
            guest_address(code, address, temporaries)?;
            load(*width, 0)?
        }
        _ => return Err("unsupported expression"),
    };
    code.instruction(&instruction);
    if matches!(value, Expr::Bin(..)) && matches!(value.width(), W::B8 | W::B16) {
        code.instruction(&Instruction::I32Const(value.width().mask() as i32));
        code.instruction(&Instruction::I32And);
    }
    Ok(())
}

fn sign_extend_i32(code: &mut wasm_encoder::Function, width: W) {
    match width {
        W::B8 => {
            code.instruction(&Instruction::I32Extend8S);
        }
        W::B16 => {
            code.instruction(&Instruction::I32Extend16S);
        }
        _ => {}
    }
}

fn shift_i64(code: &mut wasm_encoder::Function, op: BinOp) {
    // Wasm masks the count to six bits. The IR saturates shifts at 64.
    code.instruction(&Instruction::LocalSet(SHIFT_COUNT));
    code.instruction(&Instruction::LocalSet(SHIFT_VALUE));
    code.instruction(&Instruction::LocalGet(SHIFT_COUNT));
    code.instruction(&Instruction::I64Const(64));
    code.instruction(&Instruction::I64GeU);
    code.instruction(&Instruction::If(BlockType::Result(ValType::I64)));
    if op == BinOp::Sar {
        code.instruction(&Instruction::LocalGet(SHIFT_VALUE));
        code.instruction(&Instruction::I64Const(63));
        code.instruction(&Instruction::I64ShrS);
    } else {
        code.instruction(&Instruction::I64Const(0));
    }
    code.instruction(&Instruction::Else);
    code.instruction(&Instruction::LocalGet(SHIFT_VALUE));
    code.instruction(&Instruction::LocalGet(SHIFT_COUNT));
    code.instruction(&match op {
        BinOp::Shl => Instruction::I64Shl,
        BinOp::Shr => Instruction::I64ShrU,
        BinOp::Sar => Instruction::I64ShrS,
        _ => unreachable!(),
    });
    code.instruction(&Instruction::End);
}

fn guest_address(
    code: &mut wasm_encoder::Function,
    address: &Expr,
    temporaries: &BTreeMap<Tmp, u32>,
) -> Result<(), &'static str> {
    code.instruction(&Instruction::LocalGet(BASE));
    expression(code, address, temporaries)?;
    code.instruction(&Instruction::I32Add);
    Ok(())
}

fn fpu(
    code: &mut wasm_encoder::Function,
    operation: &Fpu,
    temporaries: &BTreeMap<Tmp, u32>,
) -> Result<(), &'static str> {
    match operation {
        Fpu::Init => {
            code.instruction(&Instruction::LocalGet(CONTEXT));
            code.instruction(&Instruction::Call(FNINIT));
        }
        Fpu::Load(FLoc::F32(address)) => {
            code.instruction(&Instruction::LocalGet(CONTEXT));
            guest_address(code, address, temporaries)?;
            code.instruction(&Instruction::I32Load(ctx_slot(0)));
            code.instruction(&Instruction::Call(FLD32));
        }
        Fpu::Store {
            dst: FLoc::F32(address),
            pop,
        } => {
            guest_address(code, address, temporaries)?;
            code.instruction(&Instruction::LocalGet(CONTEXT));
            code.instruction(&Instruction::Call(FST32));
            code.instruction(&Instruction::I32Store(ctx_slot(0)));
            if *pop {
                code.instruction(&Instruction::LocalGet(CONTEXT));
                code.instruction(&Instruction::Call(FPOP));
            }
        }
        Fpu::Ldcw(word) => {
            code.instruction(&Instruction::LocalGet(CONTEXT));
            expression(code, word, temporaries)?;
            code.instruction(&Instruction::Call(FLDCW));
        }
        Fpu::Load(src) => fpu_operand(code, 0, 0, src, 0, temporaries)?,
        Fpu::Store { dst, pop } => fpu_operand(code, 1, 0, dst, u8::from(*pop), temporaries)?,
        Fpu::Compare { src, pops } => fpu_operand(code, 2, 0, src, *pops, temporaries)?,
        Fpu::Arith { op, dst, src, pop } => {
            let operation = match op {
                FArith::Add => 3,
                FArith::Mul => 4,
                FArith::Sub => 5,
                FArith::SubR => 6,
                FArith::Div => 7,
                FArith::DivR => 8,
            };
            fpu_operand(code, operation, *dst, src, u8::from(*pop), temporaries)?;
        }
        Fpu::Xch(src) => fpu_operand(code, 9, 0, &FLoc::St(*src), 0, temporaries)?,
        Fpu::Free(src) => fpu_operand(code, 10, 0, &FLoc::St(*src), 0, temporaries)?,
        Fpu::Unary(op) => fpu_misc(
            code,
            match op {
                FUnary::Chs => 0,
                FUnary::Abs => 1,
                FUnary::Sqrt => 2,
                FUnary::Sin => 3,
                FUnary::Cos => 4,
                FUnary::Rndint => 5,
                FUnary::F2xm1 => 6,
                FUnary::Tst => 7,
                FUnary::Xam => 8,
            },
        ),
        Fpu::Binary(op) => fpu_misc(
            code,
            match op {
                FBinary::Patan => 9,
                FBinary::Yl2x => 10,
                FBinary::Scale => 11,
                FBinary::Prem => 12,
                FBinary::Prem1 => 13,
                FBinary::Ptan => 14,
            },
        ),
        Fpu::IncStp => fpu_misc(code, 15),
        Fpu::DecStp => fpu_misc(code, 16),
        Fpu::Clex => fpu_misc(code, 17),
        Fpu::Const(value) => fpu_misc(
            code,
            match value {
                FConst::One => 32,
                FConst::Zero => 33,
                FConst::Pi => 34,
                FConst::L2e => 35,
                FConst::L2t => 36,
                FConst::Lg2 => 37,
                FConst::Ln2 => 38,
            },
        ),
        Fpu::Stenv(address) | Fpu::Ldenv(address) | Fpu::Save(address) | Fpu::Rstor(address) => {
            let opcode = match operation {
                Fpu::Stenv(_) => 11,
                Fpu::Ldenv(_) => 12,
                Fpu::Save(_) => 13,
                Fpu::Rstor(_) => 14,
                _ => unreachable!(),
            };
            code.instruction(&Instruction::LocalGet(CONTEXT));
            code.instruction(&Instruction::I32Const(opcode));
            code.instruction(&Instruction::I32Const(0));
            guest_address(code, address, temporaries)?;
            code.instruction(&Instruction::Call(X87_MEMORY));
        }
    }
    Ok(())
}

fn fpu_misc(code: &mut wasm_encoder::Function, operation: i32) {
    code.instruction(&Instruction::LocalGet(CONTEXT));
    code.instruction(&Instruction::I32Const(operation));
    code.instruction(&Instruction::Call(X87_MISC));
}

fn fpu_operand(
    code: &mut wasm_encoder::Function,
    operation: i32,
    dst: u8,
    location: &FLoc,
    pops: u8,
    temporaries: &BTreeMap<Tmp, u32>,
) -> Result<(), &'static str> {
    code.instruction(&Instruction::LocalGet(CONTEXT));
    code.instruction(&Instruction::I32Const(operation));
    if let FLoc::St(src) = location {
        if dst > 7 || *src > 7 || (dst != 0 && *src != 0) {
            return Err("unsupported x87 register operands");
        }
        code.instruction(&Instruction::I32Const(i32::from(dst)));
        code.instruction(&Instruction::I32Const(i32::from(*src)));
        code.instruction(&Instruction::Call(X87_REGISTER));
    } else {
        if dst != 0 {
            return Err("x87 memory operation must target st(0)");
        }
        let (format, address) = match location {
            FLoc::F32(address) => (0, address),
            FLoc::F64(address) => (1, address),
            FLoc::I16(address) => (2, address),
            FLoc::I32(address) => (3, address),
            FLoc::I64(address) if operation <= 1 => (4, address),
            FLoc::F80(address) if operation <= 1 => (5, address),
            _ => return Err("unsupported x87 operand"),
        };
        code.instruction(&Instruction::I32Const(format));
        guest_address(code, address, temporaries)?;
        code.instruction(&Instruction::Call(X87_MEMORY));
    }
    for _ in 0..pops {
        code.instruction(&Instruction::LocalGet(CONTEXT));
        code.instruction(&Instruction::Call(FPOP));
    }
    Ok(())
}

const ESP: u32 = REG0 + 4;
const EBP: u32 = REG0 + 5;

fn push_return_address(code: &mut wasm_encoder::Function, address: u32) {
    // Put the guest return address on the guest stack before the call.
    code.instruction(&Instruction::LocalGet(ESP));
    code.instruction(&Instruction::I32Const(4));
    code.instruction(&Instruction::I32Sub);
    code.instruction(&Instruction::LocalSet(ESP));
    code.instruction(&Instruction::LocalGet(BASE));
    code.instruction(&Instruction::LocalGet(ESP));
    code.instruction(&Instruction::I32Add);
    code.instruction(&Instruction::I32Const(address as i32));
    code.instruction(&Instruction::I32Store(ctx_slot(0)));
}

fn count_temporaries(statements: &[Stmt]) -> usize {
    statements
        .iter()
        .map(|statement| match statement {
            Stmt::Let(..) => 1,
            Stmt::If(_, body) | Stmt::Loop(body) => count_temporaries(body),
            _ => 0,
        })
        .sum()
}

fn is_guest_interrupt(reason: &str) -> bool {
    let Some((mnemonic, _)) = reason.split_once(" at 0x") else {
        return false;
    };
    mnemonic.starts_with("software interrupt ")
        || matches!(mnemonic, "int" | "int1" | "int3" | "into")
}

fn emit_statements(
    code: &mut wasm_encoder::Function,
    statements: &[Stmt],
    temporaries: &mut BTreeMap<Tmp, u32>,
    wide_start: u32,
    address: u32,
    break_depth: Option<u32>,
) -> Result<(), &'static str> {
    for stmt in statements {
        match stmt {
            Stmt::Let(tmp, value) => {
                if let Expr::Bin(BinOp::SDiv, W::B64, dividend, divisor) = value {
                    sync_out(code);  // the runtime reports a fault with the guest state
                    code.instruction(&Instruction::LocalGet(CONTEXT));
                    code.instruction(&Instruction::I32Const(address as i32));
                    expression(code, dividend, temporaries)?;
                    expression(code, divisor, temporaries)?;
                    code.instruction(&Instruction::Call(SIGNED_DIVIDE));
                } else {
                    expression(code, value, temporaries)?;
                }
                let base = if value.width() == W::B64 {
                    wide_start
                } else {
                    FIRST_TEMP
                };
                let next = base + temporaries.len() as u32;
                let slot = *temporaries.entry(*tmp).or_insert(next);
                if (slot >= wide_start) != (value.width() == W::B64) {
                    return Err("temporary changes WebAssembly type");
                }
                code.instruction(&Instruction::LocalSet(slot));
            }
            Stmt::SetReg(reg, value) => {
                let local = reg_local(reg)?;
                let (keep, mask, shift) = match reg.part {
                    Part::D => (0, -1, 0),
                    Part::X => (0xFFFF_0000u32 as i32, 0xFFFF, 0),
                    Part::L => (0xFFFF_FF00u32 as i32, 0xFF, 0),
                    Part::H => (0xFFFF_00FFu32 as i32, 0xFF, 8),
                };
                if reg.part == Part::D {
                    expression(code, value, temporaries)?;
                } else {
                    code.instruction(&Instruction::LocalGet(local));
                    code.instruction(&Instruction::I32Const(keep));
                    code.instruction(&Instruction::I32And);
                    expression(code, value, temporaries)?;
                    code.instruction(&Instruction::I32Const(mask));
                    code.instruction(&Instruction::I32And);
                    if shift != 0 {
                        code.instruction(&Instruction::I32Const(shift));
                        code.instruction(&Instruction::I32Shl);
                    }
                    code.instruction(&Instruction::I32Or);
                }
                code.instruction(&Instruction::LocalSet(local));
            }
            Stmt::SetFlag(flag, value) => {
                expression(code, value, temporaries)?;
                code.instruction(&Instruction::I32Const(1));
                code.instruction(&Instruction::I32And);
                code.instruction(&Instruction::LocalSet(flag_local(*flag)));
            }
            Stmt::Store(address, value) => {
                guest_address(code, address, temporaries)?;
                expression(code, value, temporaries)?;
                code.instruction(&store(value.width(), 0)?);
            }
            Stmt::Fpu(operation) => fpu(code, operation, temporaries)?,
            Stmt::If(condition, body) => {
                expression(code, condition, temporaries)?;
                code.instruction(&Instruction::If(BlockType::Empty));
                emit_statements(
                    code,
                    body,
                    &mut temporaries.clone(),
                    wide_start,
                    address,
                    break_depth.map(|depth| depth + 1),
                )?;
                code.instruction(&Instruction::End);
            }
            Stmt::Loop(body) => {
                code.instruction(&Instruction::Block(BlockType::Empty));
                code.instruction(&Instruction::Loop(BlockType::Empty));
                emit_statements(
                    code,
                    body,
                    &mut temporaries.clone(),
                    wide_start,
                    address,
                    Some(1),
                )?;
                code.instruction(&Instruction::Br(0));
                code.instruction(&Instruction::End);
                code.instruction(&Instruction::End);
            }
            Stmt::BreakIf(condition) => {
                let depth = break_depth.ok_or("break outside instruction loop")?;
                expression(code, condition, temporaries)?;
                code.instruction(&Instruction::BrIf(depth));
            }
            Stmt::Trap(reason)
                if matches!(reason.as_str(), "divide by zero" | "divide overflow") =>
            {
                sync_out(code);
                code.instruction(&Instruction::LocalGet(CONTEXT));
                code.instruction(&Instruction::I32Const(address as i32));
                code.instruction(&Instruction::Call(DIVIDE_ERROR));
                code.instruction(&Instruction::Unreachable);
            }
            Stmt::Trap(reason) if is_guest_interrupt(reason) => {
                sync_out(code);
                code.instruction(&Instruction::LocalGet(CONTEXT));
                code.instruction(&Instruction::I32Const(address as i32));
                code.instruction(&Instruction::Call(INTERRUPT));
                code.instruction(&Instruction::Unreachable);
            }
            _ => return Err("unsupported statement"),
        }
    }
    Ok(())
}

fn dispatch(code: &mut wasm_encoder::Function, from: u32, owning_frame: Option<u32>) {
    code.instruction(&Instruction::I32Const(from as i32));
    if let Some(excluded_origin) = owning_frame {
        code.instruction(&Instruction::I32Const(excluded_origin as i32));
        code.instruction(&Instruction::Call(DISPATCH_OWNED));
        // The runtime already executed the continuation for this activation.
        code.instruction(&Instruction::If(BlockType::Empty));
        code.instruction(&Instruction::Return);
        code.instruction(&Instruction::End);
    } else {
        code.instruction(&Instruction::Call(DISPATCH));
    }
}

/// Direct calls between lifted functions. Calls to `hooked` addresses (the runtime's own
/// replacements) and calls from frames that own a resume site still go through the runtime.
pub struct Links {
    index: BTreeMap<u32, u32>,
    hooked: BTreeSet<u32>,
}

impl Links {
    fn direct(&self, target: u32) -> Option<u32> {
        if self.hooked.contains(&target) {
            return None;
        }
        self.index.get(&target).copied()
    }
}

#[derive(Clone, Copy)]
struct TransferContext<'a> {
    pc: u32,
    loop_depth: u32,
    from: u32,
    owning_frame: Option<u32>,
    links: Option<&'a Links>,
}

impl TransferContext<'_> {
    fn direct(&self, target: u32) -> Option<u32> {
        if self.owning_frame.is_some() {
            return None;
        }
        self.links?.direct(target)
    }
}

fn transfer(
    code: &mut wasm_encoder::Function,
    cases: &BTreeMap<u32, u32>,
    context: TransferContext,
    target: u32,
) {
    if let Some(&case) = cases.get(&target) {
        code.instruction(&Instruction::I32Const(case as i32));
        code.instruction(&Instruction::LocalSet(context.pc));
        code.instruction(&Instruction::Br(context.loop_depth));
    } else {
        sync_out(code);
        code.instruction(&Instruction::LocalGet(CONTEXT));
        if let Some(callee) = context.direct(target) {
            code.instruction(&Instruction::Call(callee));
        } else {
            code.instruction(&Instruction::I32Const(target as i32));
            dispatch(code, context.from, context.owning_frame);
        }
        code.instruction(&Instruction::Return);
    }
}

fn writes_seh_head(statements: &[Stmt]) -> bool {
    fn is_fs_head(address: &Expr, temporaries: &BTreeSet<Tmp>) -> bool {
        match address {
            Expr::FsBase => true,
            Expr::Tmp(_, tmp) => temporaries.contains(tmp),
            Expr::Bin(BinOp::Add, W::B32, left, right) => {
                matches!(right.as_ref(), Expr::Const(_, 0)) && is_fs_head(left, temporaries)
            }
            _ => false,
        }
    }
    let mut temporaries = BTreeSet::new();
    for statement in statements {
        match statement {
            Stmt::Let(tmp, value) => {
                if is_fs_head(value, &temporaries) {
                    temporaries.insert(*tmp);
                } else {
                    temporaries.remove(tmp);
                }
            }
            Stmt::Store(address, value)
                if value.width() == W::B32 && is_fs_head(address, &temporaries) =>
            {
                return true;
            }
            _ => {}
        }
    }
    false
}

fn emit_body(
    function: &Function,
    resume_sites: &BTreeSet<u32>,
    links: Option<&Links>,
) -> Result<wasm_encoder::Function, String> {
    if resume_sites.is_empty() && (function.except_block || !function.continuations.is_empty()) {
        return Err(format!(
            "exception frame at {:#010x} requires resume lowering",
            function.addr
        ));
    }
    let owns_frame = !resume_sites.is_empty()
        && (!function.continuations.is_empty()
            || function
                .insns
                .iter()
                .any(|insn| writes_seh_head(&insn.stmts)));
    let owning_frame = if owns_frame {
        let mut origins = function
            .insns
            .iter()
            .map(|insn| insn.addr)
            .filter(|address| resume_sites.contains(address));
        let excluded_origin = origins.next().unwrap_or(0);
        if origins.next().is_some() {
            return Err(format!(
                "exception frame at {:#010x} has multiple resume origins",
                function.addr
            ));
        }
        Some(excluded_origin)
    } else {
        None
    };
    let local_count = function
        .insns
        .iter()
        .map(|i| count_temporaries(&i.stmts))
        .max()
        .unwrap_or(0);
    let local_count = u32::try_from(local_count).map_err(|_| "too many temporaries")?;
    let call_target = local_count
        .checked_add(FIRST_TEMP)
        .ok_or("too many temporaries")?;
    let wide_start = call_target.checked_add(1).ok_or("too many temporaries")?;
    let pc = wide_start
        .checked_add(local_count)
        .ok_or("too many temporaries")?;
    let mut code = wasm_encoder::Function::new([
        (2, ValType::I64),
        (local_count + 1 + (FIRST_TEMP - BASE), ValType::I32),
        (local_count, ValType::I64),
        (1, ValType::I32),
    ]);

    // Cases begin at transfer targets or after a break in linear emission.
    if function
        .insns
        .windows(2)
        .any(|pair| pair[0].addr >= pair[1].addr)
    {
        return Err(format!(
            "function {:#010x}: instruction addresses must increase",
            function.addr
        ));
    }
    let addresses: BTreeSet<_> = function.insns.iter().map(|insn| insn.addr).collect();
    let mut starts = BTreeSet::from([function.addr]);
    for (index, insn) in function.insns.iter().enumerate() {
        let following = function.insns.get(index + 1).map(|next| next.addr);
        if let Some(next) = following {
            if !matches!(insn.term, Term::Next) || next != insn.next() {
                starts.insert(next);
            }
        }
        let mut add = |target| {
            if addresses.contains(&target) {
                starts.insert(target);
            }
        };
        match &insn.term {
            Term::Jump(target) => add(*target),
            Term::Branch(_, target) => {
                add(*target);
                if following != Some(insn.next()) {
                    add(insn.next());
                }
            }
            Term::JumpTable { cases, .. } => cases.iter().copied().for_each(&mut add),
            Term::Call { ret, .. } | Term::CallIndirect { ret, .. } => add(*ret),
            Term::Next if following != Some(insn.next()) => add(insn.next()),
            _ => {}
        }
    }
    if !addresses.contains(&function.addr) {
        return Err(format!(
            "function {:#010x} has no entry instruction",
            function.addr
        ));
    }
    if let Some(&first) = addresses.first() {
        starts.insert(first);
    }
    let n = u32::try_from(starts.len()).map_err(|_| "too many blocks")?;
    let cases: BTreeMap<_, _> = starts.into_iter().zip(0..n).collect();
    code.instruction(&Instruction::Call(GUEST_BASE));
    code.instruction(&Instruction::LocalSet(BASE));
    sync_in(&mut code);
    code.instruction(&Instruction::I32Const(cases[&function.addr] as i32));
    code.instruction(&Instruction::LocalSet(pc));
    code.instruction(&Instruction::Block(BlockType::Empty));
    code.instruction(&Instruction::Loop(BlockType::Empty));
    for _ in 0..n {
        code.instruction(&Instruction::Block(BlockType::Empty));
    }
    code.instruction(&Instruction::LocalGet(pc));
    code.instruction(&Instruction::BrTable(
        (0..n).collect::<Vec<_>>().into(),
        n + 1,
    ));

    let mut case = 0;
    let mut temporaries = BTreeMap::new();
    for (index, insn) in function.insns.iter().enumerate() {
        if let Some(&next_case) = cases.get(&insn.addr) {
            code.instruction(&Instruction::End);
            case = next_case;
        }
        let depth = n - case - 1;
        let context = TransferContext {
            pc,
            loop_depth: depth,
            from: insn.addr,
            owning_frame,
            links,
        };
        let nested = TransferContext {
            loop_depth: depth + 1,
            ..context
        };
        let at = |error| {
            if insn.text.is_empty() {
                format!("{:#010x}: {error}", insn.addr)
            } else {
                format!("{:#010x} ({}): {error}", insn.addr, insn.text)
            }
        };
        temporaries.clear();
        emit_statements(
            &mut code,
            &insn.stmts,
            &mut temporaries,
            wide_start,
            insn.addr,
            None,
        )
        .map_err(at)?;
        if resume_sites.contains(&insn.addr) {
            match &insn.term {
                Term::Jump(target) | Term::Call { target, .. } => {
                    code.instruction(&Instruction::I32Const(*target as i32));
                }
                Term::JumpIndirect(target) | Term::CallIndirect { target, .. } => {
                    expression(&mut code, target, &temporaries).map_err(at)?;
                }
                _ => return Err(at("manifest resume site must be a call or jump")),
            }
            // The guest has already restored the owning frame's EBP.
            // This transfer abandons the current stack without pushing a return address.
            sync_out(&mut code);
            code.instruction(&Instruction::LocalGet(EBP));
            code.instruction(&Instruction::I32Const(insn.addr as i32));
            code.instruction(&Instruction::Call(RESUME));
            code.instruction(&Instruction::Unreachable);
            continue;
        }
        let fallthrough = match &insn.term {
            Term::Next => Some(insn.next()),
            Term::Branch(condition, target) => {
                expression(&mut code, condition, &temporaries).map_err(at)?;
                code.instruction(&Instruction::If(BlockType::Empty));
                transfer(&mut code, &cases, nested, *target);
                code.instruction(&Instruction::End);
                Some(insn.next())
            }
            Term::Jump(target) => {
                transfer(&mut code, &cases, context, *target);
                None
            }
            Term::JumpIndirect(target) => {
                expression(&mut code, target, &temporaries).map_err(at)?;
                code.instruction(&Instruction::LocalSet(call_target));
                sync_out(&mut code);
                code.instruction(&Instruction::LocalGet(CONTEXT));
                code.instruction(&Instruction::LocalGet(call_target));
                dispatch(&mut code, insn.addr, owning_frame);
                code.instruction(&Instruction::Return);
                None
            }
            Term::JumpTable {
                target,
                cases: targets,
            } => {
                expression(&mut code, target, &temporaries).map_err(at)?;
                code.instruction(&Instruction::LocalSet(call_target));
                for target in targets.iter().copied().collect::<BTreeSet<_>>() {
                    code.instruction(&Instruction::LocalGet(call_target));
                    code.instruction(&Instruction::I32Const(target as i32));
                    code.instruction(&Instruction::I32Eq);
                    code.instruction(&Instruction::If(BlockType::Empty));
                    transfer(&mut code, &cases, nested, target);
                    code.instruction(&Instruction::End);
                }
                code.instruction(&Instruction::Unreachable);
                None
            }
            Term::Call { target, ret, .. } if context.direct(*target).is_some() => {
                push_return_address(&mut code, *ret);
                sync_out(&mut code);
                code.instruction(&Instruction::LocalGet(CONTEXT));
                code.instruction(&Instruction::Call(context.direct(*target).unwrap()));
                sync_in(&mut code);
                Some(*ret)
            }
            Term::Call { ret, .. } | Term::CallIndirect { ret, .. } => {
                match &insn.term {
                    Term::Call { target, .. } => {
                        code.instruction(&Instruction::I32Const(*target as i32));
                    }
                    Term::CallIndirect { target, .. } => {
                        expression(&mut code, target, &temporaries).map_err(at)?;
                    }
                    _ => unreachable!(),
                }
                // An indirect target can use ESP, so read it before the push.
                code.instruction(&Instruction::LocalSet(call_target));
                push_return_address(&mut code, *ret);
                sync_out(&mut code);
                code.instruction(&Instruction::LocalGet(CONTEXT));
                code.instruction(&Instruction::LocalGet(call_target));
                dispatch(&mut code, insn.addr, owning_frame);
                sync_in(&mut code);
                Some(*ret)
            }
            Term::Ret(cleanup) => {
                code.instruction(&Instruction::LocalGet(ESP));
                code.instruction(&Instruction::I32Const(4 + i32::from(*cleanup)));
                code.instruction(&Instruction::I32Add);
                code.instruction(&Instruction::LocalSet(ESP));
                sync_out(&mut code);
                code.instruction(&Instruction::Return);
                None
            }
            Term::Stop
                if insn.stmts.last().is_some_and(
                    |stmt| matches!(stmt, Stmt::Trap(reason) if is_guest_interrupt(reason)),
                ) =>
            {
                None
            }
            Term::Stop => return Err(at("unsupported stop instruction")),
        };
        if let Some(target) = fallthrough {
            if function.insns.get(index + 1).map(|next| next.addr) != Some(target) {
                transfer(&mut code, &cases, context, target);
            }
        }
    }
    code.instruction(&Instruction::End);
    code.instruction(&Instruction::End);
    code.instruction(&Instruction::Unreachable);
    code.instruction(&Instruction::End);
    Ok(code)
}

/// Emit an experimental module with one export per guest entry address.
pub fn emit(functions: &[Function]) -> Result<Vec<u8>, String> {
    emit_with_resume_sites(functions, &BTreeSet::new())
}

/// Use the resume addresses validated against the selected executable's manifest.
pub fn emit_with_resume_sites(
    functions: &[Function],
    resume_sites: &BTreeSet<u32>,
) -> Result<Vec<u8>, String> {
    let addrs: Vec<u32> = functions.iter().map(|f| f.addr).collect();
    emit_module(&addrs, &mut |i| Ok(Cow::Borrowed(&functions[i])), resume_sites, None, &mut |_| {})
}

/// As [`emit_with_resume_sites`], with direct calls between the lifted functions except to
/// `hooked` (the addresses the runtime replaces), which still go through `h4.dispatch`.
pub fn emit_linked(
    functions: &[Function],
    resume_sites: &BTreeSet<u32>,
    hooked: &BTreeSet<u32>,
) -> Result<Vec<u8>, String> {
    emit_linked_progress(functions, resume_sites, hooked, &mut |_| {})
}

/// [`emit_linked`], telling `progress` how far it is (0 to 1) every few hundred functions.
pub fn emit_linked_progress(
    functions: &[Function],
    resume_sites: &BTreeSet<u32>,
    hooked: &BTreeSet<u32>,
    progress: &mut dyn FnMut(f64),
) -> Result<Vec<u8>, String> {
    let addrs: Vec<u32> = functions.iter().map(|f| f.addr).collect();
    emit_linked_each(&addrs, &mut |i| Ok(Cow::Borrowed(&functions[i])), resume_sites, hooked, progress)
}

/// [`emit_linked_progress`] for the functions at `addrs` (ascending, as the slice's would be),
/// each made only when its turn comes (`function(i)`) and dropped after: the lifted IR of a whole
/// game takes most of a gigabyte, too much for a phone's browser.
pub fn emit_linked_each<'a>(
    addrs: &[u32],
    function: &mut dyn FnMut(usize) -> Result<Cow<'a, Function>, String>,
    resume_sites: &BTreeSet<u32>,
    hooked: &BTreeSet<u32>,
    progress: &mut dyn FnMut(f64),
) -> Result<Vec<u8>, String> {
    let mut index = BTreeMap::new();
    for (position, addr) in addrs.iter().enumerate() {
        let callee = u32::try_from(position + HOST_FUNCTIONS).map_err(|_| "too many functions")?;
        index.insert(*addr, callee);
    }
    let links = Links { index, hooked: hooked.clone() };
    emit_module(addrs, function, resume_sites, Some(&links), progress)
}

/// [`emit_linked_each`], the module handed to `write` in pieces (its offset, the bytes) as it is
/// made, never whole in memory: its start (the sections before the code, and the code section's
/// size) last. Gives the module's size.
pub fn emit_linked_each_to<'a>(
    addrs: &[u32],
    function: &mut dyn FnMut(usize) -> Result<Cow<'a, Function>, String>,
    resume_sites: &BTreeSet<u32>,
    hooked: &BTreeSet<u32>,
    progress: &mut dyn FnMut(f64),
    write: &mut dyn FnMut(usize, &[u8]),
) -> Result<usize, String> {
    let mut index = BTreeMap::new();
    for (position, addr) in addrs.iter().enumerate() {
        let callee = u32::try_from(position + HOST_FUNCTIONS).map_err(|_| "too many functions")?;
        index.insert(*addr, callee);
    }
    let links = Links { index, hooked: hooked.clone() };
    emit_module_to(addrs, function, resume_sites, Some(&links), progress, write)
}

fn emit_module<'a>(
    addrs: &[u32],
    function: &mut dyn FnMut(usize) -> Result<Cow<'a, Function>, String>,
    resume_sites: &BTreeSet<u32>,
    links: Option<&Links>,
    progress: &mut dyn FnMut(f64),
) -> Result<Vec<u8>, String> {
    let mut out = Vec::new();
    emit_module_to(addrs, function, resume_sites, links, progress, &mut |at, bytes| {
        if out.len() < at + bytes.len() {
            out.resize(at + bytes.len(), 0);
        }
        out[at..at + bytes.len()].copy_from_slice(bytes);
    })?;
    Ok(out)
}

fn emit_module_to<'a>(
    addrs: &[u32],
    function: &mut dyn FnMut(usize) -> Result<Cow<'a, Function>, String>,
    resume_sites: &BTreeSet<u32>,
    links: Option<&Links>,
    progress: &mut dyn FnMut(f64),
    write: &mut dyn FnMut(usize, &[u8]),
) -> Result<usize, String> {
    let mut module = Module::new();
    let mut types = TypeSection::new();
    types.ty().function([], [ValType::I32]);
    types.ty().function([ValType::I32; 3], []);
    types.ty().function([ValType::I32], []);
    types.ty().function([ValType::I32; 2], []);
    types.ty().function([ValType::I32], [ValType::I32]);
    types.ty().function(
        [ValType::I32, ValType::I32, ValType::I64, ValType::I64],
        [ValType::I64],
    );
    types.ty().function([ValType::I32; 4], []);
    types.ty().function([ValType::I32; 4], [ValType::I32]);
    module.section(&types);

    let mut imports = ImportSection::new();
    imports.import("h4", "guest_base", EntityType::Function(0));
    imports.import("h4", "dispatch", EntityType::Function(1));
    imports.import("h4", "fninit", EntityType::Function(2));
    imports.import("h4", "fld32", EntityType::Function(3));
    imports.import("h4", "fst32", EntityType::Function(4));
    imports.import("h4", "fstsw", EntityType::Function(4));
    imports.import("h4", "fpop", EntityType::Function(2));
    imports.import("h4", "divide_error", EntityType::Function(3));
    imports.import("h4", "signed_divide", EntityType::Function(5));
    imports.import("h4", "fstcw", EntityType::Function(4));
    imports.import("h4", "fldcw", EntityType::Function(3));
    imports.import("h4", "x87_memory", EntityType::Function(6));
    imports.import("h4", "x87_register", EntityType::Function(6));
    imports.import("h4", "x87_misc", EntityType::Function(3));
    imports.import("h4", "interrupt", EntityType::Function(3));
    imports.import("h4", "resume", EntityType::Function(1));
    imports.import("h4", "dispatch_owned", EntityType::Function(7));
    imports.import(
        "h4",
        "memory",
        EntityType::Memory(MemoryType {
            minimum: 1,
            maximum: Some(65536),
            memory64: false,
            shared: true,
            page_size_log2: None,
        }),
    );
    module.section(&imports);

    let mut declarations = FunctionSection::new();
    let mut exports = ExportSection::new();
    let mut entries = BTreeSet::new();
    for (position, &addr) in addrs.iter().enumerate() {
        if !entries.insert(addr) {
            return Err(format!("duplicate function at {addr:#010x}"));
        }
        let index = u32::try_from(position + HOST_FUNCTIONS).map_err(|_| "too many functions")?;
        declarations.function(2);
        exports.export(&format!("f_{addr:08x}"), ExportKind::Func, index);
    }
    module.section(&declarations).section(&exports);
    // The code section (most of the module: 150 MB) handed on in pieces as its functions are
    // made, its size and count (5-byte LEB128s) at the end with the start: never the whole code
    // in memory (a phone's browser stopped the converter).
    let mut head = module.finish();
    head.push(10); // (the code section's id)
    let size_at = head.len();
    head.extend_from_slice(&[0; 10]);
    const PIECE: usize = 8 << 20;
    let mut piece = Vec::with_capacity(PIECE + (1 << 20));
    let mut at = head.len(); // where `piece` goes
    for (position, &addr) in addrs.iter().enumerate() {
        let function = function(position)?;
        debug_assert_eq!(function.addr, addr);
        emit_body(&function, resume_sites, links)?.encode(&mut piece);
        if piece.len() >= PIECE {
            write(at, &piece);
            at += piece.len();
            piece.clear();
        }
        if (position + HOST_FUNCTIONS) % 256 == 0 {
            progress(position as f64 / addrs.len() as f64);
        }
    }
    write(at, &piece);
    let end = at + piece.len();
    let leb5 = |v: u32| -> [u8; 5] {
        [
            (v as u8) | 0x80,
            ((v >> 7) as u8) | 0x80,
            ((v >> 14) as u8) | 0x80,
            ((v >> 21) as u8) | 0x80,
            (v >> 28) as u8,
        ]
    };
    let size = u32::try_from(end - size_at - 5).map_err(|_| "too much code")?;
    let count = u32::try_from(addrs.len()).map_err(|_| "too many functions")?;
    head[size_at..size_at + 5].copy_from_slice(&leb5(size));
    head[size_at + 5..size_at + 10].copy_from_slice(&leb5(count));
    write(0, &head);
    Ok(end)
}

#[cfg(test)]
mod tests {
    use super::*;
    use rc_ir::Insn;

    #[test]
    fn rejects_unsupported_control_flow_at_guest_address() {
        let function = Function {
            addr: 0x401000,
            insns: vec![Insn {
                addr: 0x401000,
                len: 2,
                text: String::new(),
                stmts: vec![],
                term: Term::Stop,
            }],
            ..Function::default()
        };
        assert_eq!(
            emit(&[function]).unwrap_err(),
            "0x00401000: unsupported stop instruction"
        );
    }

    #[test]
    fn rejects_exception_frames_until_resume_is_lowered() {
        let function = Function {
            addr: 0x401000,
            except_block: true,
            ..Function::default()
        };
        assert_eq!(
            emit(&[function]).unwrap_err(),
            "exception frame at 0x00401000 requires resume lowering"
        );
    }
}
