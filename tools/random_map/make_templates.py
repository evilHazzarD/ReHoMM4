#!/usr/bin/env python3
# Writes the template files of ours that are made by rule (cross.txt ... race.txt: a file a
# template, its variants by the players' number and the map size) into
# h4/src/random_map/templates/ (ring.txt, hub.txt, duel.txt are kept as they are). Our own
# layouts, of the kinds popular in Heroes 3's templates (no one's template copied). Then run
# embed_templates.py.
from fractions import Fraction as F
import sys, os

class T:
    def __init__(s): s.zones=[]; s.links=[]
    def z(s, id, kind='treasure', rich=False, town=False, size=1, r=None, turn=None, x=None, y=None):
        s.zones.append((id,kind,rich,town,size,r,turn,x,y)); return id
    def l(s, a, b, guard=None): s.links.append((a,b,guard))
    def text(s):
        out=[]
        for id,kind,rich,town,size,r,turn,x,y in s.zones:
            t=f"zone {id} {kind}"+(" rich" if rich else "")+(" town" if town else "")
            if size!=1: t+=f" size={size:g}"
            if r is not None:
                fr=F(turn).limit_denominator(64) % 1
                t+=f" r={r:g} turn={fr.numerator}/{fr.denominator}" if fr.denominator>1 else f" r={r:g} turn={fr.numerator}"
            elif x is not None: t+=f" x={x:g} y={y:g}"
            out.append(t)
        for a,b,g in s.links: out.append(f"link {a} {b}"+(f" {g}" if g else ""))
        return "\n".join(out)+"\n"

SIZES={False:"small-medium", True:"large-huge"}

def cross(P,big):  # 4 corners, own zones round a rich middle
    t=T(); order=[0,2,1,3]
    c=t.z("middle",rich=True,town=True,size=1.4,r=0,turn=0)
    own=[]
    for k in range(4):
        i=order[k]; a=F(2*i+1,8)
        s=t.z(f"p{k+1}","start",size=1.2,r=0.37,turn=a)
    for i in range(4):
        a=F(2*i+1,8); o=t.z(f"own{i+1}",r=0.2,turn=a); own.append(o)
        t.l(f"p{order.index(i)+1}",o); t.l(o,c,"deadly")
    for i in range(4): t.l(own[i],own[(i+1)%4],"normal")
    if big:
        for i in range(4):
            e=t.z(f"edge{i+1}",rich=True,r=0.38,turn=F(i,4)+F(1,4))
            t.l(f"p{order.index(i)+1}",e,"strong"); t.l(f"p{order.index((i+1)%4)+1}",e,"strong")
    return t

def spider(P,big):  # legs to a rich middle, rich zones between the legs
    t=T(); c=t.z("middle",rich=True,town=True,size=1.3,r=0,turn=0)
    legs=[]
    for i in range(P):
        a=F(i,P); s=t.z(f"p{i+1}","start",size=1.2,r=0.37,turn=a)
        g=t.z(f"leg{i+1}",r=0.21,turn=a); legs.append(g)
        t.l(s,g,"weak"); t.l(g,c,"strong")
        if big:
            o=t.z(f"out{i+1}",r=0.4,turn=a+F(1,4*P)); t.l(s,o,"weak")
    for i in range(P):
        b=t.z(f"web{i+1}",rich=True,r=0.33,turn=F(2*i+1,2*P)); t.l(legs[i],b,"normal"); t.l(legs[(i+1)%P],b,"normal")
    return t

def mirror(P,big):  # two mirrored halves, two rich middles joined by an open way
    t=T(); top=t.z("north",rich=True,x=0,y=-0.17); bot=t.z("south",rich=True,town=True,x=0,y=0.17)
    t.l(top,bot,"open")
    for k,sx in ((1,-1),(2,1)):
        s=t.z(f"p{k}","start",size=1.2,x=0.33*sx,y=0)
        a=t.z(f"front{k}a",x=0.17*sx,y=-0.2); b=t.z(f"front{k}b",x=0.17*sx,y=0.2)
        t.l(s,a,"weak"); t.l(s,b,"weak"); t.l(a,top,"strong"); t.l(b,bot,"strong")
        back=t.z(f"back{k}",x=0.34*sx,y=0.3 if k==1 else -0.3, size=0.8); t.l(s,back,"weak")
        if big:
            far=t.z(f"far{k}",rich=True,x=0.34*sx,y=-0.3 if k==1 else 0.3); t.l(a if k==1 else b, far, "strong")
    return t

def diamond(P,big):  # starts on the sides' middles, corners between, a rich middle
    t=T(); c=t.z("middle",rich=True,town=True,size=1.3,x=0,y=0)
    sp=[(0,-0.36),(0,0.36),(-0.36,0),(0.36,0)]  # 2 players face each other
    for k,(x,y) in enumerate(sp): t.z(f"p{k+1}","start",size=1.1,x=x,y=y)
    corners=[(-0.22,-0.22),(0.22,-0.22),(0.22,0.22),(-0.22,0.22)]  # (in: bordering the middle)
    near={0:[0,1],1:[2,3],2:[0,3],3:[1,2]}
    for i,(x,y) in enumerate(corners): t.z(f"corner{i+1}",rich=big,x=x,y=y)
    for k in range(4):
        for i in near[k]: t.l(f"p{k+1}",f"corner{i+1}","weak" if not big else "normal")
    for i in range(4): t.l(f"corner{i+1}",c,"strong")
    if big:
        for k,(x,y) in enumerate(sp):
            px,py=(0.14,0.38 if y>0 else -0.38) if x==0 else (0.38 if x>0 else -0.38,0.14)
            p=t.z(f"pocket{k+1}",size=0.7,x=px,y=py); t.l(f"p{k+1}",p,"weak")
    return t

def allies(P,big):  # 2 against 2: each pair shares a back zone, the middle between the pairs
    t=T()
    for k,(x,y) in enumerate([(-0.33,-0.2),(0.33,0.2),(-0.33,0.2),(0.33,-0.2)]): t.z(f"p{k+1}","start",size=1.1,x=x,y=y)
    t.z("west",x=-0.36,y=0,size=0.8); t.z("east",x=0.36,y=0,size=0.8)
    for p,b in (("p1","west"),("p3","west"),("p2","east"),("p4","east")): t.l(p,b,"weak")
    t.z("middle",rich=True,town=True,x=0,y=0,size=1.2)
    t.z("north",x=0,y=-0.3); t.z("south",x=0,y=0.3)
    for p,m in (("p1","north"),("p4","north"),("p3","south"),("p2","south")): t.l(p,m,"normal")
    t.l("north","middle","strong"); t.l("south","middle","strong")
    if big:
        for i,(x,y) in enumerate([(-0.16,-0.36),(0.16,0.36)]):
            z=t.z(f"rich{i+1}",rich=True,x=x,y=y,size=0.8); t.l("north" if i==0 else "south",z,"deadly")
    return t

def triangle(P,big):  # 3 starts, a zone between each two, a rich middle
    t=T(); c=t.z("middle",rich=True,town=True,size=1.3,r=0,turn=0)
    for i in range(3): t.z(f"p{i+1}","start",size=1.2,r=0.37,turn=F(i,3))
    for i in range(3):
        a=t.z(f"side{i+1}",r=0.33,turn=F(2*i+1,6)); t.l(f"p{i+1}",a,"normal"); t.l(f"p{(i+1)%3+1}",a,"normal"); t.l(a,c,"strong")
        if big:
            o=t.z(f"inner{i+1}",r=0.18,turn=F(i,3)); t.l(f"p{i+1}",o,"weak"); t.l(o,c,"strong")
    return t

def corridor(P,big):  # two players at the ends of a line of zones, pockets on its sides
    t=T(); xs=[-0.36,-0.18,0,0.18,0.36] if not big else [-0.38,-0.25,-0.12,0,0.12,0.25,0.38]
    n=len(xs); ids=[]
    for i,x in enumerate(xs):
        if i==0: ids.append(t.z("p1","start",size=1.2,x=x,y=0))
        elif i==n-1: ids.append(t.z("p2","start",size=1.2,x=x,y=0))
        else: ids.append(t.z(f"way{i}",rich=(i==n//2),town=(i==n//2),x=x,y=0))
    for i in range(n-1): t.l(ids[i],ids[i+1],"strong" if i in (n//2-1,n//2) else "normal")
    for i in range(1,n-1):
        for sy,tag in ((-0.28,"n"),(0.28,"s")):
            p=t.z(f"pocket{i}{tag}",rich=abs(i-n//2)<=1,size=0.8,x=xs[i],y=sy); t.l(ids[i],p,"strong" if abs(i-n//2)<=1 else "weak")
    return t

def pockets(P,big):  # each start with private pockets behind it, a ring of shared zones in front
    t=T(); ring=[]
    for i in range(P):
        a=F(i,P); s=t.z(f"p{i+1}","start",size=1.1,r=0.3,turn=a)
        for j,d in enumerate((-1,1)):
            p=t.z(f"pocket{i+1}{'ab'[j]}",size=0.7,r=0.38,turn=a+F(d,5*P)); t.l(s,p,"weak")
        m=t.z(f"ring{i+1}",rich=big,r=0.15 if P<=3 else 0.17,turn=a+F(1,2*P)); ring.append(m)
    for i in range(P):
        t.l(f"p{i+1}",ring[i],"normal"); t.l(f"p{(i+1)%P+1}",ring[i],"normal")
    if P>2:
        c=t.z("middle",rich=True,town=True,size=0.8,r=0,turn=0)
        for m in ring: t.l(m,c,"strong")
    return t

def clash(P,big):  # the starts close round the middle, the riches out by the edges
    t=T()
    for i in range(P): t.z(f"p{i+1}","start",size=1.1,r=0.17,turn=F(i,P))
    for i in range(P):
        t.z(f"out{i+1}",rich=True,town=(i%2==0 and big),r=0.37,turn=F(i,P))
        t.z(f"gap{i+1}",r=0.34,turn=F(2*i+1,2*P))
    for i in range(P):
        t.l(f"p{i+1}",f"out{i+1}","strong")
        t.l(f"gap{i+1}",f"out{i+1}","normal")
        t.l(f"gap{i+1}",f"out{(i+1)%P+1}","normal") if P>2 or i==0 else None
        t.l(f"p{i+1}",f"p{(i+1)%P+1}","normal") if P>2 or i==0 else None
    return t

def web(P,big):  # two rings of small zones, many ways
    t=T(); inner=[]; outer=[]
    for i in range(P): t.z(f"p{i+1}","start",size=1.1,r=0.37,turn=F(i,P))
    for i in range(P):
        o=t.z(f"outer{i+1}",size=0.8,r=0.37,turn=F(2*i+1,2*P)); outer.append(o)
        n=t.z(f"inner{i+1}",rich=(i%2==1),size=0.8,r=0.2,turn=F(i,P)); inner.append(n)
    for i in range(P):
        t.l(f"p{i+1}",outer[i],"weak"); t.l(outer[i],f"p{(i+1)%P+1}","weak")
        t.l(f"p{i+1}",inner[i],"normal"); t.l(inner[i],inner[(i+1)%P],"normal")
        t.l(outer[i],inner[i],"strong")
    c=t.z("middle",rich=True,town=True,size=0.9 if not big else 1.2,r=0,turn=0)
    for n in inner: t.l(n,c,"strong")
    return t

def race(P,big):  # each player a lane of richer and richer zones to the middle
    t=T(); c=t.z("middle",rich=True,town=True,r=0,turn=0)
    steps=[0.26,0.15] if not big else [0.29,0.21,0.13]
    for i in range(P):
        a=F(i,P); prev=t.z(f"p{i+1}","start",r=0.37,turn=a)
        for j,r in enumerate(steps):
            z=t.z(f"lane{i+1}{'abc'[j]}",rich=(j==len(steps)-1),r=r,turn=a); t.l(prev,z,["weak","normal","strong"][j] if len(steps)==3 else ["weak","strong"][j]); prev=z
        t.l(prev,c,"deadly")
    for i in range(P):  # a back way between neighbours' first zones
        t.l(f"lane{i+1}a",f"lane{(i+1)%P+1}a","strong") if P>2 or i==0 else None
    return t

def hourglass(P,big):  # two halves joined through one narrow rich middle
    t=T(); mid=t.z("neck",rich=True,town=True,size=0.8,x=0,y=0)
    for k,sx in ((1,-1),(2,1)):
        s=t.z(f"p{k}","start",size=1.3,x=0.33*sx,y=0)
        a=t.z(f"bulb{k}a",x=0.2*sx,y=-0.28); b=t.z(f"bulb{k}b",x=0.2*sx,y=0.28)
        g=t.z(f"gate{k}",x=0.14*sx,y=0,size=0.8)
        t.l(s,a,"weak"); t.l(s,b,"weak"); t.l(a,g,"normal"); t.l(b,g,"normal"); t.l(g,mid,"deadly")
        if big:
            for tag,y in (("c",-0.38),("d",0.38)):
                e=t.z(f"bulb{k}{tag}",rich=True,x=0.36*sx,y=y,size=0.8); t.l(a if y<0 else b,e,"strong")
    return t

def star(P,big):  # the starts on a star's points, an inner ring between them
    t=T(); inner=[]
    for i in range(P): t.z(f"p{i+1}","start",size=1.1,r=0.38,turn=F(i,P))
    for i in range(P):
        n=t.z(f"inner{i+1}",rich=(i%2==1 or P==3),r=0.2,turn=F(2*i+1,2*P)); inner.append(n)
    for i in range(P):
        t.l(f"p{i+1}",inner[i],"normal"); t.l(f"p{i+1}",inner[i-1],"normal")
        t.l(inner[i],inner[(i+1)%P],"strong")
        if big:
            o=t.z(f"tip{i+1}",r=0.38,turn=F(2*i+1,2*P),size=0.8); t.l(o,inner[i],"strong")
    if big:
        c=t.z("middle",rich=True,town=True,r=0,turn=0,size=0.8)
        for n in inner: t.l(n,c,"deadly")
    return t

def fortress(P,big):  # each start behind one strongly guarded gate; a wide land outside
    # the outer ring: starts and their yards; the inner one: their gates and the fields between
    t=T(); field=[]
    for i in range(P):
        a=F(i,P); h=F(1,2*P)
        s=t.z(f"p{i+1}","start",size=1.2,r=0.36,turn=a)
        y=t.z(f"yard{i+1}",size=0.8,r=0.38,turn=a+h); t.l(s,y,"weak")
        g=t.z(f"gate{i+1}",size=0.8,r=0.2,turn=a); t.l(s,g,"strong")
        f=t.z(f"field{i+1}",rich=big,r=0.22,turn=a+h); field.append(f); t.l(g,f,"normal")
    for i in range(P):
        t.l(field[i],f"gate{(i+1)%P+1}","normal") if P>2 or i==0 else None
    if P>2:
        c=t.z("middle",rich=True,town=True,r=0,turn=0,size=0.8)
        for f in field: t.l(f,c,"strong")
    return t

def ladder(P,big):  # two players, two parallel lines of zones with rungs between them
    t=T(); n=3 if not big else 4
    xs=[-0.24+0.48*i/(n-1) for i in range(n)]
    t.z("p1","start",size=1.2,x=-0.38,y=0); t.z("p2","start",size=1.2,x=0.38,y=0)
    for row,y in (("n",-0.2),("s",0.2)):
        prev="p1"
        for i,x in enumerate(xs):
            z=t.z(f"{row}{i+1}",rich=(i in (n//2,(n-1)//2)),x=round(x,3),y=y); t.l(prev,z,"normal" if i else "weak"); prev=z
        t.l(prev,"p2","weak")
    for i in range(n):
        if i%2==1 or n==3 and i==1: t.l(f"n{i+1}",f"s{i+1}","strong")
    return t

def labyrinth(P,big):  # a winding line through a grid of zones, few ways across
    t=T(); g=3 if not big else 4
    step=0.66/(g-1); pos=lambda i,j:(round(-0.33+step*j,3),round(-0.33+step*i,3))
    order=[(i, j if i%2==0 else g-1-j) for i in range(g) for j in range(g)]  # a snake
    ids=[]
    for k,(i,j) in enumerate(order):
        x,y=pos(i,j)
        if k==0: ids.append(t.z("p1","start",x=x,y=y))
        elif k==len(order)-1: ids.append(t.z("p2","start",x=x,y=y))
        else: ids.append(t.z(f"cell{k}",rich=(k==len(order)//2),town=(k==len(order)//2),x=x,y=y))
    for k in range(len(ids)-1): t.l(ids[k],ids[k+1],"strong" if abs(k-len(ids)//2)<=1 else "normal")
    # one short cut across the turns, deadly guarded
    t.l(ids[1],ids[2*g-2],"deadly")
    return t

def frontier(P,big):  # two lands of three zones each, a line of guarded border zones between them
    t=T(); n=3 if not big else 4
    ys=[round(-0.3+0.6*i/(n-1),3) for i in range(n)]
    for k,sx in ((1,-1),(2,1)):
        t.z(f"p{k}","start",size=1.2,x=0.34*sx,y=0)
        for tag,y in (("a",-0.3),("b",0.3)):
            h=t.z(f"home{k}{tag}",x=0.3*sx,y=y); t.l(f"p{k}",h,"weak")
    for i,y in enumerate(ys):
        b=t.z(f"border{i+1}",rich=(i%2==1),town=(i==n//2),size=0.8,x=0,y=y)
        for k,sx in ((1,-1),(2,1)):
            near=f"p{k}" if abs(y)<0.15 else f"home{k}{'a' if y<0 else 'b'}"
            t.l(near,b,"strong")
    return t

def three_on_three(P,big):  # six players, two teams of three, each team's starts in a row
    t=T()
    for k,(x,y) in enumerate([(-0.34,-0.3),(0.34,0.3),(-0.34,0),(0.34,0),(-0.34,0.3),(0.34,-0.3)]):
        t.z(f"p{k+1}","start",x=x,y=y)
    t.z("west",x=-0.36,y=-0.15 if False else 0.15,size=0.6); t.z("east",x=0.36,y=-0.15,size=0.6)
    for p in ("p3","p5"): t.l(p,"west","weak")
    for p in ("p4","p6"): t.l(p,"east","weak")
    mids=[]
    for i,y in enumerate((-0.3,0,0.3)):
        m=t.z(f"front{i+1}",rich=(i==1),town=(i==1),x=0,y=y); mids.append(m)
    for i,(w,e) in enumerate((("p1","p6"),("p3","p4"),("p5","p2"))):
        t.l(w,mids[i],"strong"); t.l(e,mids[i],"strong")
    t.l(mids[0],mids[1],"normal"); t.l(mids[1],mids[2],"normal")
    t.l("p1","p3","weak"); t.l("p6","p4","weak")
    return t

def spiral(P,big):  # the starts on a spiral's arm, each a step nearer the rich middle's town
    t=T(); n=P*(2 if big else 1); prev=None; c=t.z("middle",rich=True,town=True,r=0,turn=0)
    k=0
    for i in range(n+P):
        r=round(0.38-0.2*i/(n+P-1),3); a=F(i,P)+F(i,5*(n+P))
        if i%(1+n//P)==0 and k<P: z=t.z(f"p{k+1}","start",r=r,turn=a); k+=1
        else: z=t.z(f"arm{i+1}",rich=(i>=(n+P)*2//3),r=r,turn=a)
        if prev: t.l(prev,z,"normal")
        prev=z
    t.l(prev,c,"strong"); t.l(f"p1",prev if P>2 else c,"deadly") if False else None
    return t

def wheel(P,big):  # a hub, spokes to the starts on the rim, rim zones between the starts
    t=T(); hub=t.z("hub",rich=True,town=True,size=0.9,r=0,turn=0)
    for i in range(P):
        a=F(i,P); s=t.z(f"p{i+1}","start",r=0.37,turn=a)
        sp=t.z(f"spoke{i+1}",r=0.19,turn=a,size=0.8); t.l(s,sp,"weak"); t.l(sp,hub,"strong")
        rim=t.z(f"rim{i+1}",rich=big,r=0.38,turn=a+F(1,2*P)); t.l(s,rim,"normal")
    for i in range(P): t.l(f"rim{i+1}",f"p{(i+1)%P+1}","normal")
    return t

def twin_rings(P,big):  # two players, each in a ring of own zones; the rings touch at two gates
    t=T()
    for k,sx in ((1,-1),(2,1)):
        t.z(f"p{k}","start",size=1.2,x=0.34*sx,y=0)
        n=t.z(f"north{k}",x=0.22*sx,y=-0.26); so=t.z(f"south{k}",x=0.22*sx,y=0.26)
        t.l(f"p{k}",n,"weak"); t.l(f"p{k}",so,"weak")
        if big:
            e=t.z(f"inner{k}",rich=True,x=0.16*sx,y=0,size=0.8); t.l(n,e,"normal"); t.l(so,e,"normal")
    g1=t.z("gate_north",rich=True,size=0.8,x=0,y=-0.3); g2=t.z("gate_south",rich=True,town=True,size=0.8,x=0,y=0.3)
    for k in (1,2): t.l(f"north{k}",g1,"strong"); t.l(f"south{k}",g2,"strong")
    return t

def crown(P,big):  # the starts along the bottom, the crown's rich points along the top
    t=T(); xs=[round(-0.33+0.66*i/(P-1),3) for i in range(P)]
    base=[]
    for i,x in enumerate(xs):
        t.z(f"p{i+1}","start",x=x,y=0.3); b=t.z(f"band{i+1}",x=x,y=0.02,size=0.8); base.append(b); t.l(f"p{i+1}",b,"weak")
    for i in range(P-1):
        pt=t.z(f"point{i+1}",rich=True,town=(i==(P-1)//2),x=round((xs[i]+xs[i+1])/2,3),y=-0.3)
        t.l(base[i],pt,"strong"); t.l(base[i+1],pt,"strong")
    for i in range(P-1): t.l(base[i],base[i+1],"normal")
    return t

def chain(P,big):  # the starts in a line, each two neighbours sharing a link zone
    t=T(); n=P; xs=[round(-0.34+0.68*i/(n-1),3) for i in range(n)]
    for i,x in enumerate(xs):
        y=-0.2 if i%2 else 0.2
        t.z(f"p{i+1}","start",x=x,y=y)
        if big: o=t.z(f"own{i+1}",x=x,y=-y*1.8 if abs(y*1.8)<=0.4 else -0.36,size=0.8); t.l(f"p{i+1}",o,"weak")
    for i in range(n-1):
        l=t.z(f"link{i+1}",rich=(i==(n-2)//2),town=(i==(n-2)//2),x=round((xs[i]+xs[i+1])/2,3),y=0)
        t.l(f"p{i+1}",l,"normal"); t.l(f"p{i+2}",l,"normal")
    return t

def arena(P,big):  # one wide rich middle every start opens on; little else
    t=T(); c=t.z("arena",rich=True,town=True,size=2.2 if big else 1.6,r=0,turn=0)
    for i in range(P):
        a=F(i,P); s=t.z(f"p{i+1}","start",r=0.37,turn=a); t.l(s,c,"strong")
        if big:
            b=t.z(f"back{i+1}",r=0.38,turn=a+F(1,2*P),size=0.7); t.l(s,b,"weak")
    return t

def gauntlet(P,big):  # each start's way out: zone after zone, guarded more each time
    t=T(); c=t.z("prize",rich=True,town=True,r=0,turn=0,size=0.8)
    steps=[0.26] if not big else [0.29,0.2]
    for i in range(P):
        a=F(i,P); prev=t.z(f"p{i+1}","start",r=0.38,turn=a)
        for j,r in enumerate(steps):
            z=t.z(f"trial{i+1}{'ab'[j]}",rich=(j==len(steps)-1),r=r,turn=a+F(1,4*P)*(j%2)); t.l(prev,z,["normal","strong"][j]); prev=z
        t.l(prev,c,"deadly")
    for i in range(P): t.l(f"trial{i+1}a",f"trial{(i+1)%P+1}a","strong") if P>2 else None
    if P==2: t.l("trial1a","trial2a","strong")
    return t

def treasury(P,big):  # a ring of small rich vaults round the middle, each deadly guarded
    t=T(); c=t.z("middle",town=True,r=0,turn=0)
    for i in range(P):
        a=F(i,P); s=t.z(f"p{i+1}","start",r=0.37,turn=a); t.l(s,c,"normal")
    vaults=P*(2 if big else 1)
    for v in range(vaults):
        z=t.z(f"vault{v+1}",rich=True,size=0.5,r=0.27,turn=F(2*v+1,2*vaults)); t.l(c,z,"deadly")
        t.l(f"p{v*P//vaults+1}",z,"strong") if big else None
    return t

def blitz(P,big):  # two players near each other, one small zone each, an open way between
    t=T()
    t.z("p1","start",x=-0.2,y=0); t.z("p2","start",x=0.2,y=0)
    t.z("north",rich=True,x=0,y=-0.3,size=0.8); t.z("south",x=0,y=0.3,size=0.8)
    t.l("p1","p2","weak"); t.l("p1","north","normal"); t.l("p2","north","normal")
    t.l("p1","south","weak"); t.l("p2","south","weak")
    if big:
        for k,sx in ((1,-1),(2,1)):
            b=t.z(f"back{k}",x=0.38*sx,y=0); t.l(f"p{k}",b,"weak")
    return t

def checkerboard(P,big):  # a grid of zones, the starts in its corners
    t=T(); g=3 if not big else 4
    step=0.7/(g-1); pos=lambda i,j:(round(-0.35+step*j,3),round(-0.35+step*i,3))
    corners=[(0,0),(g-1,g-1),(0,g-1),(g-1,0)]
    ids={}
    k=0
    for i in range(g):
        for j in range(g):
            x,y=pos(i,j)
            if (i,j) in corners[:P]:
                k=corners.index((i,j)); ids[i,j]=t.z(f"p{k+1}","start",x=x,y=y)
            else:
                centre=(g%2==1 and i==j==g//2) or (g%2==0 and i in (g//2-1,g//2) and j in (g//2-1,g//2))
                ids[i,j]=t.z(f"square{i+1}{j+1}",rich=centre,town=centre and (g%2==1 or (i,j)==(g//2,g//2)),x=x,y=y)
    for i in range(g):
        for j in range(g):
            if j+1<g: t.l(ids[i,j],ids[i,j+1],"normal" if (i+j)%2 else "strong")
            if i+1<g: t.l(ids[i,j],ids[i+1,j],"strong" if (i+j)%2 else "normal")
    return t

def horseshoe(P,big):  # two players at a horseshoe's ends, the riches round its bend
    t=T(); n=5 if not big else 7
    prev=None; ids=[]
    for k in range(n):
        a=F(1,8)+F(3,4)*F(k,n-1)  # from one end round the bottom to the other
        if k==0: z=t.z("p1","start",r=0.33,turn=F(3,8)-F(3,4)*F(k,n-1) if False else a)
        elif k==n-1: z=t.z("p2","start",r=0.33,turn=a)
        else: z=t.z(f"bend{k}",rich=(k==n//2),town=(k==n//2),r=0.33,turn=a)
        if prev: t.l(prev,z,"strong" if k in (n//2,n//2+1) else "normal")
        prev=z; ids.append(z)
    gap=t.z("gap",size=0.7,r=0.1,turn=F(0)); t.l("p1",gap,"deadly"); t.l("p2",gap,"deadly")
    return t

PACKS=[
 ("cross.txt","Cross","# Cross: four corners round a rich middle with a town; each start's own zone on the way in,\n# the own zones linked in a ring; on large maps rich edges between the corners.\n",cross,range(2,5)),
 ("spider.txt","Spider","# Spider: each start a leg to a rich middle with a town; rich zones between the legs.\n",spider,range(2,7)),
 ("mirror.txt","Mirror","# Mirror: two players, mirrored halves; two rich middles joined by an open way.\n",mirror,range(2,3)),
 ("diamond.txt","Diamond","# Diamond: the starts on the sides' middles, corner zones between them, a rich middle.\n",diamond,range(2,5)),
 ("allies.txt","Allies","# Allies: two against two (set the teams: players 1 and 3, 2 and 4); each pair shares a\n# back zone, the middle and its town between the pairs.\n",allies,range(4,5)),
 ("triangle.txt","Triangle","# Triangle: three players, a zone between each two, a rich middle with a town.\n",triangle,range(3,4)),
 ("corridor.txt","Corridor","# Corridor: two players at the ends of a line of zones, pockets on its sides, the richest\n# (and a town) in the middle.\n",corridor,range(2,3)),
 ("pockets.txt","Pockets","# Pockets: each start with two private pockets behind it; shared zones in front, between\n# neighbours (and a rich middle for 3 players and more).\n",pockets,range(2,7)),
 ("clash.txt","Clash","# Clash: the starts close round the middle, the riches out by the map's edges.\n",clash,range(2,7)),
 ("web.txt","Web","# Web: two rings of small zones and many ways between them, a rich middle.\n",web,range(2,7)),
 ("hourglass.txt","Hourglass","# Hourglass: two players; each half's zones meet at its gate, the gates at one narrow rich\n# middle with a town, deadly guarded.\n",hourglass,range(2,3)),
 ("star.txt","Star","# Star: the starts on a star's points, an inner ring of zones between them (and on large\n# maps a rich middle).\n",star,range(3,7)),
 ("fortress.txt","Fortress","# Fortress: each start behind one strongly guarded gate and its own yard; a wide land\n# outside (and a rich middle for 3 players and more).\n",fortress,range(2,7)),
 ("ladder.txt","Ladder","# Ladder: two players, two parallel lines of zones between them with guarded rungs across.\n",ladder,range(2,3)),
 ("labyrinth.txt","Labyrinth","# Labyrinth: two players at the ends of a winding line through a grid of zones; one deadly\n# short cut.\n",labyrinth,range(2,3)),
 ("frontier.txt","Frontier","# Frontier: two lands of three zones, a line of strongly guarded border zones between them.\n",frontier,range(2,3)),
 ("three_on_three.txt","Three on three","# Three on three: six players, two teams (set them: players 1, 3, 5 and 2, 4, 6), each\n# team's starts in a row on its side, the fronts between.\n",three_on_three,range(6,7)),
 ("spiral.txt","Spiral","# Spiral: the starts along a spiral's arm to the rich middle, each nearer it than the last.\n",spiral,range(2,5)),
 ("wheel.txt","Wheel","# Wheel: the starts on the rim, a spoke from each to a rich hub with a town; rim zones\n# between the starts.\n",wheel,range(2,7)),
 ("twin_rings.txt","Twin rings","# Twin rings: two players, each in a ring of own zones; the rings touch at two rich gates.\n",twin_rings,range(2,3)),
 ("crown.txt","Crown","# Crown: the starts along the bottom, a band between them, the rich points along the top.\n",crown,range(2,6)),
 ("chain.txt","Chain","# Chain: the starts in a line, each two neighbours sharing a zone between them.\n",chain,range(2,7)),
 ("arena.txt","Arena","# Arena: every start opens on one wide rich middle with a town; a quick, fighting map.\n",arena,range(2,7)),
 ("gauntlet.txt","Gauntlet","# Gauntlet: each start's way to the prize in the middle, zone after zone guarded more each time.\n",gauntlet,range(2,7)),
 ("treasury.txt","Treasury","# Treasury: small rich vaults round the middle, each deadly guarded.\n",treasury,range(2,7)),
 ("blitz.txt","Blitz","# Blitz: two players close to each other, few zones: a short game.\n",blitz,range(2,3)),
 ("checkerboard.txt","Checkerboard","# Checkerboard: a grid of zones, the starts in its corners, the riches in its middle.\n",checkerboard,range(2,5)),
 ("horseshoe.txt","Horseshoe","# Horseshoe: two players at a horseshoe's ends, the riches round its bend; a deadly short cut\n# across its opening.\n",horseshoe,range(2,3)),
 ("race.txt","Race","# Race: each player a lane of richer zones to a deadly guarded middle with a town.\n",race,range(2,7)),
]

def main(out_dir):
    for file,name,head,fn,players in PACKS:
        blocks=[head]
        single=fn in (cross,diamond)  # 4 starts: the ones past the players' number are treasure zones
        for P in ([4] if single else players):
            for big in (False,True):
                t=fn(P,big)
                pl=f"{players.start}-{players.stop-1}" if single else str(P)
                # room enough: 13 zones at most on a small map, 26 on a medium one
                n=len(t.zones)
                sizes=SIZES[big]
                if not big and n>26: continue
                if not big and n>13: sizes="medium"
                blocks.append(f"\nname {name}\nplayers {pl}\nsizes {sizes}\n\n"+t.text())
        open(os.path.join(out_dir,file),"w").write("".join(blocks))
main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'h4', 'src', 'random_map', 'templates'))
