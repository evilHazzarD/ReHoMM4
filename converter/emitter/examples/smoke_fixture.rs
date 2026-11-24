// Pipe the fixture into a file, then instantiate it with the web loader.
use std::{
    collections::BTreeSet,
    io::{self, Write},
};

use rc_ir::{
    BinOp, EAX, ESP, Expr, FLoc, Flag, Fpu, Function, Insn, Part, Reg, Stmt, Term, UnOp, W,
};

fn main() -> io::Result<()> {
    let addr = 0x401000;
    let ah = Reg {
        n: EAX,
        part: Part::H,
    };
    let code = rehomm4_emit_wasm::emit_with_resume_sites(
        &[
            Function {
                addr,
                continuations: vec![addr + 6],
                insns: vec![
                    Insn {
                        addr,
                        len: 1,
                        text: String::new(),
                        stmts: vec![
                            Stmt::Fpu(Fpu::Init),
                            Stmt::Fpu(Fpu::Load(FLoc::F32(Expr::Const(W::B32, 0x24)))),
                            Stmt::Fpu(Fpu::Store {
                                dst: FLoc::F32(Expr::Const(W::B32, 0x28)),
                                pop: true,
                            }),
                            Stmt::Let(7, Expr::Load(W::B32, Box::new(Expr::Const(W::B32, 0x20)))),
                            Stmt::SetFlag(
                                Flag::Pf,
                                Expr::un(UnOp::Parity, W::B8, Expr::Tmp(W::B32, 7).trunc(W::B8)),
                            ),
                            Stmt::SetFlag(Flag::Cf, Expr::Flag(Flag::Df)),
                            Stmt::SetFlag(
                                Flag::Of,
                                Expr::bin(
                                    BinOp::Slt,
                                    W::B8,
                                    Expr::c(W::B8, 0x80),
                                    Expr::c(W::B8, 1),
                                ),
                            ),
                            Stmt::SetReg(
                                Reg::d(EAX),
                                Expr::Bin(
                                    BinOp::Add,
                                    W::B32,
                                    Box::new(Expr::Reg(Reg::d(EAX))),
                                    Box::new(Expr::Tmp(W::B32, 7)),
                                ),
                            ),
                            Stmt::SetReg(ah, Expr::c(W::B8, 0xab)),
                            Stmt::Store(Expr::c32(0x2d), Expr::Reg(ah)),
                            Stmt::Fpu(Fpu::Ldcw(Expr::c(W::B16, 0x27f))),
                            Stmt::Store(Expr::c32(0x58), Expr::FpuControl),
                            Stmt::Fpu(Fpu::Load(FLoc::F64(Expr::c32(0x60)))),
                            Stmt::Fpu(Fpu::Arith {
                                op: rc_ir::FArith::Add,
                                dst: 0,
                                src: FLoc::F64(Expr::c32(0x68)),
                                pop: false,
                            }),
                            Stmt::Fpu(Fpu::Load(FLoc::St(0))),
                            Stmt::Fpu(Fpu::Arith {
                                op: rc_ir::FArith::Add,
                                dst: 1,
                                src: FLoc::St(0),
                                pop: true,
                            }),
                            Stmt::Fpu(Fpu::Unary(rc_ir::FUnary::Chs)),
                            Stmt::Fpu(Fpu::Store {
                                dst: FLoc::F64(Expr::c32(0x70)),
                                pop: true,
                            }),
                            Stmt::Let(10, Expr::load(W::B32, Expr::c32(0x40)).sext(W::B64)),
                            Stmt::If(
                                Expr::Tmp(W::B64, 10).is_zero(),
                                vec![Stmt::Trap("divide by zero".into())],
                            ),
                            Stmt::Let(
                                11,
                                Expr::bin(
                                    BinOp::SDiv,
                                    W::B64,
                                    Expr::c(W::B64, (-17_i64) as u64),
                                    Expr::Tmp(W::B64, 10),
                                ),
                            ),
                            Stmt::Store(Expr::c32(0x44), Expr::Tmp(W::B64, 11).trunc(W::B32)),
                            Stmt::Store(
                                Expr::c32(0x48),
                                Expr::bin(
                                    BinOp::SRem,
                                    W::B64,
                                    Expr::c(W::B64, (-17_i64) as u64),
                                    Expr::Tmp(W::B64, 10),
                                )
                                .trunc(W::B32),
                            ),
                            Stmt::Let(
                                9,
                                Expr::bin(
                                    BinOp::Mul,
                                    W::B64,
                                    Expr::c32((-2i32) as u32).sext(W::B64),
                                    Expr::c32(0x70000000).sext(W::B64),
                                ),
                            ),
                            Stmt::Store(Expr::c32(0x30), Expr::Tmp(W::B64, 9).trunc(W::B32)),
                            Stmt::Store(
                                Expr::c32(0x38),
                                Expr::bin(
                                    BinOp::Sar,
                                    W::B64,
                                    Expr::Tmp(W::B64, 9),
                                    Expr::c(W::B64, 65),
                                )
                                .trunc(W::B32),
                            ),
                            Stmt::Store(
                                Expr::c32(0x3c),
                                Expr::bin(
                                    BinOp::Shl,
                                    W::B64,
                                    Expr::Tmp(W::B64, 9),
                                    Expr::c(W::B64, 64),
                                )
                                .trunc(W::B32),
                            ),
                            Stmt::Store(
                                Expr::c32(0x34),
                                Expr::bin(
                                    BinOp::Shr,
                                    W::B64,
                                    Expr::Tmp(W::B64, 9),
                                    Expr::c(W::B64, 32),
                                )
                                .trunc(W::B32),
                            ),
                            Stmt::SetFlag(
                                Flag::Of,
                                Expr::bin(
                                    BinOp::Ne,
                                    W::B64,
                                    Expr::Tmp(W::B64, 9),
                                    Expr::Tmp(W::B64, 9).trunc(W::B32).sext(W::B64),
                                ),
                            ),
                            Stmt::SetReg(Reg::d(rc_ir::ECX), Expr::c32(3)),
                            Stmt::Loop(vec![
                                Stmt::Let(
                                    8,
                                    Expr::bin(
                                        BinOp::Sub,
                                        W::B32,
                                        Expr::Reg(Reg::d(rc_ir::ECX)),
                                        Expr::c32(1),
                                    ),
                                ),
                                Stmt::SetReg(Reg::d(rc_ir::ECX), Expr::Tmp(W::B32, 8)),
                                Stmt::If(
                                    Expr::Tmp(W::B32, 8).is_zero(),
                                    vec![Stmt::BreakIf(Expr::c32(1))],
                                ),
                            ]),
                        ],
                        term: Term::Next,
                    },
                    Insn {
                        addr: addr + 1,
                        len: 5,
                        text: String::new(),
                        stmts: Vec::new(),
                        term: Term::CallIndirect {
                            target: Expr::load(W::B32, Expr::Reg(Reg::d(ESP))),
                            ret: addr + 6,
                        },
                    },
                    Insn {
                        addr: addr + 6,
                        len: 1,
                        text: String::new(),
                        stmts: Vec::new(),
                        term: Term::Branch(
                            Expr::bin(BinOp::Ne, W::B8, Expr::Flag(Flag::Df), Expr::c(W::B8, 0)),
                            addr + 8,
                        ),
                    },
                    Insn {
                        addr: addr + 7,
                        len: 1,
                        text: String::new(),
                        stmts: Vec::new(),
                        term: Term::Jump(addr + 9),
                    },
                    Insn {
                        addr: addr + 8,
                        len: 1,
                        text: String::new(),
                        stmts: Vec::new(),
                        term: Term::JumpTable {
                            target: Expr::c32(addr + 12),
                            cases: vec![addr + 12],
                        },
                    },
                    Insn {
                        addr: addr + 9,
                        len: 1,
                        text: String::new(),
                        stmts: vec![
                            Stmt::SetReg(Reg::d(EAX), Expr::c32(0)),
                            Stmt::SetReg(Reg::d(rc_ir::ECX), Expr::c32(3)),
                        ],
                        term: Term::Next,
                    },
                    Insn {
                        addr: addr + 10,
                        len: 1,
                        text: String::new(),
                        stmts: vec![Stmt::SetReg(
                            Reg::d(rc_ir::ECX),
                            Expr::bin(
                                BinOp::Sub,
                                W::B32,
                                Expr::Reg(Reg::d(rc_ir::ECX)),
                                Expr::c32(1),
                            ),
                        )],
                        term: Term::Branch(Expr::Reg(Reg::d(rc_ir::ECX)), addr + 10),
                    },
                    Insn {
                        addr: addr + 11,
                        len: 1,
                        text: String::new(),
                        stmts: Vec::new(),
                        term: Term::Ret(4),
                    },
                    Insn {
                        addr: addr + 12,
                        len: 1,
                        text: String::new(),
                        stmts: Vec::new(),
                        term: Term::Ret(8),
                    },
                ],
                ..Function::default()
            },
            Function {
                addr: 0x403000,
                insns: vec![Insn {
                    addr: 0x403000,
                    len: 2,
                    text: "int 6".into(),
                    stmts: vec![
                        Stmt::Trap("software interrupt 0x6 at 0x00403000".into()),
                        Stmt::Trap("int at 0x00403000".into()),
                    ],
                    term: Term::Stop,
                }],
                ..Function::default()
            },
            Function {
                addr: 0x404000,
                insns: vec![Insn {
                    addr: 0x404000,
                    len: 2,
                    text: "call [esp]".into(),
                    stmts: vec![],
                    term: Term::CallIndirect {
                        target: Expr::load(W::B32, Expr::Reg(Reg::d(ESP))),
                        ret: 0x404002,
                    },
                }],
                ..Function::default()
            },
        ],
        &BTreeSet::from([0x404000]),
    )
    .map_err(io::Error::other)?;
    io::stdout().write_all(&code)
}
