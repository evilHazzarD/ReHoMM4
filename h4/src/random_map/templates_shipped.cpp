// Made by tools/random_map/embed_templates.py from templates/*.txt: do not edit.
#include "template.h"

namespace h4::random_map {

const ShippedTemplate kShippedTemplates[] = {
    {"allies.txt",
     R"tpl(# Allies: two against two (players 1 and 3, 2 and 4: the teams line); each pair shares a
# back zone, the middle and its town between the pairs.
)tpl"
     R"tpl(
name Allies
players 4
teams 1,3 2,4
sizes small-medium

zone p1 start size=1.1 x=-0.33 y=-0.2
zone p2 start size=1.1 x=0.33 y=0.2
zone p3 start size=1.1 x=-0.33 y=0.2
zone p4 start size=1.1 x=0.33 y=-0.2
zone west treasure size=0.8 x=-0.36 y=0
zone east treasure size=0.8 x=0.36 y=0
zone middle treasure rich town size=1.2 x=0 y=0
zone north treasure x=0 y=-0.3
zone south treasure x=0 y=0.3
link p1 west weak
link p3 west weak
link p2 east weak
link p4 east weak
link p1 north normal
link p4 north normal
link p3 south normal
link p2 south normal
link north middle strong
link south middle strong
)tpl"
     R"tpl(
name Allies
players 4
teams 1,3 2,4
sizes large-huge

zone p1 start size=1.1 x=-0.33 y=-0.2
zone p2 start size=1.1 x=0.33 y=0.2
zone p3 start size=1.1 x=-0.33 y=0.2
zone p4 start size=1.1 x=0.33 y=-0.2
zone west treasure size=0.8 x=-0.36 y=0
zone east treasure size=0.8 x=0.36 y=0
zone middle treasure rich town size=1.2 x=0 y=0
zone north treasure x=0 y=-0.3
zone south treasure x=0 y=0.3
zone rich1 treasure rich size=0.8 x=-0.16 y=-0.36
zone rich2 treasure rich size=0.8 x=0.16 y=0.36
link p1 west weak
link p3 west weak
link p2 east weak
link p4 east weak
link p1 north normal
link p4 north normal
link p3 south normal
link p2 south normal
link north middle strong
link south middle strong
link north rich1 deadly
link south rich2 deadly
)tpl"},
    {"arena.txt",
     R"tpl(# Arena: every start opens on one wide rich middle with a town; a quick, fighting map.
)tpl"
     R"tpl(
name Arena
players 2
sizes small-medium

zone arena treasure rich town size=1.6 r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/2
link p1 arena strong
link p2 arena strong
)tpl"
     R"tpl(
name Arena
players 2
sizes large-huge

zone arena treasure rich town size=2.2 r=0 turn=0
zone p1 start r=0.37 turn=0
zone back1 treasure size=0.7 r=0.38 turn=1/4
zone p2 start r=0.37 turn=1/2
zone back2 treasure size=0.7 r=0.38 turn=3/4
link p1 arena strong
link p1 back1 weak
link p2 arena strong
link p2 back2 weak
)tpl"
     R"tpl(
name Arena
players 3
sizes small-medium

zone arena treasure rich town size=1.6 r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/3
zone p3 start r=0.37 turn=2/3
link p1 arena strong
link p2 arena strong
link p3 arena strong
)tpl"
     R"tpl(
name Arena
players 3
sizes large-huge

zone arena treasure rich town size=2.2 r=0 turn=0
zone p1 start r=0.37 turn=0
zone back1 treasure size=0.7 r=0.38 turn=1/6
zone p2 start r=0.37 turn=1/3
zone back2 treasure size=0.7 r=0.38 turn=1/2
zone p3 start r=0.37 turn=2/3
zone back3 treasure size=0.7 r=0.38 turn=5/6
link p1 arena strong
link p1 back1 weak
link p2 arena strong
link p2 back2 weak
link p3 arena strong
link p3 back3 weak
)tpl"
     R"tpl(
name Arena
players 4
sizes small-medium

zone arena treasure rich town size=1.6 r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/4
zone p3 start r=0.37 turn=1/2
zone p4 start r=0.37 turn=3/4
link p1 arena strong
link p2 arena strong
link p3 arena strong
link p4 arena strong
)tpl"
     R"tpl(
name Arena
players 4
sizes large-huge

zone arena treasure rich town size=2.2 r=0 turn=0
zone p1 start r=0.37 turn=0
zone back1 treasure size=0.7 r=0.38 turn=1/8
zone p2 start r=0.37 turn=1/4
zone back2 treasure size=0.7 r=0.38 turn=3/8
zone p3 start r=0.37 turn=1/2
zone back3 treasure size=0.7 r=0.38 turn=5/8
zone p4 start r=0.37 turn=3/4
zone back4 treasure size=0.7 r=0.38 turn=7/8
link p1 arena strong
link p1 back1 weak
link p2 arena strong
link p2 back2 weak
link p3 arena strong
link p3 back3 weak
link p4 arena strong
link p4 back4 weak
)tpl"
     R"tpl(
name Arena
players 5
sizes small-medium

zone arena treasure rich town size=1.6 r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/5
zone p3 start r=0.37 turn=2/5
zone p4 start r=0.37 turn=3/5
zone p5 start r=0.37 turn=4/5
link p1 arena strong
link p2 arena strong
link p3 arena strong
link p4 arena strong
link p5 arena strong
)tpl"
     R"tpl(
name Arena
players 5
sizes large-huge

zone arena treasure rich town size=2.2 r=0 turn=0
zone p1 start r=0.37 turn=0
zone back1 treasure size=0.7 r=0.38 turn=1/10
zone p2 start r=0.37 turn=1/5
zone back2 treasure size=0.7 r=0.38 turn=3/10
zone p3 start r=0.37 turn=2/5
zone back3 treasure size=0.7 r=0.38 turn=1/2
zone p4 start r=0.37 turn=3/5
zone back4 treasure size=0.7 r=0.38 turn=7/10
zone p5 start r=0.37 turn=4/5
zone back5 treasure size=0.7 r=0.38 turn=9/10
link p1 arena strong
link p1 back1 weak
link p2 arena strong
link p2 back2 weak
link p3 arena strong
link p3 back3 weak
link p4 arena strong
link p4 back4 weak
link p5 arena strong
link p5 back5 weak
)tpl"
     R"tpl(
name Arena
players 6
sizes small-medium

zone arena treasure rich town size=1.6 r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/6
zone p3 start r=0.37 turn=1/3
zone p4 start r=0.37 turn=1/2
zone p5 start r=0.37 turn=2/3
zone p6 start r=0.37 turn=5/6
link p1 arena strong
link p2 arena strong
link p3 arena strong
link p4 arena strong
link p5 arena strong
link p6 arena strong
)tpl"
     R"tpl(
name Arena
players 6
sizes large-huge

zone arena treasure rich town size=2.2 r=0 turn=0
zone p1 start r=0.37 turn=0
zone back1 treasure size=0.7 r=0.38 turn=1/12
zone p2 start r=0.37 turn=1/6
zone back2 treasure size=0.7 r=0.38 turn=1/4
zone p3 start r=0.37 turn=1/3
zone back3 treasure size=0.7 r=0.38 turn=5/12
zone p4 start r=0.37 turn=1/2
zone back4 treasure size=0.7 r=0.38 turn=7/12
zone p5 start r=0.37 turn=2/3
zone back5 treasure size=0.7 r=0.38 turn=3/4
zone p6 start r=0.37 turn=5/6
zone back6 treasure size=0.7 r=0.38 turn=11/12
link p1 arena strong
link p1 back1 weak
link p2 arena strong
link p2 back2 weak
link p3 arena strong
link p3 back3 weak
link p4 arena strong
link p4 back4 weak
link p5 arena strong
link p5 back5 weak
link p6 arena strong
link p6 back6 weak
)tpl"},
    {"blitz.txt",
     R"tpl(# Blitz: two players close to each other, few zones: a short game.
)tpl"
     R"tpl(
name Blitz
players 2
sizes small-medium

zone p1 start x=-0.2 y=0
zone p2 start x=0.2 y=0
zone north treasure rich size=0.8 x=0 y=-0.3
zone south treasure size=0.8 x=0 y=0.3
link p1 p2 weak
link p1 north normal
link p2 north normal
link p1 south weak
link p2 south weak
)tpl"
     R"tpl(
name Blitz
players 2
sizes large-huge

zone p1 start x=-0.2 y=0
zone p2 start x=0.2 y=0
zone north treasure rich size=0.8 x=0 y=-0.3
zone south treasure size=0.8 x=0 y=0.3
zone back1 treasure x=-0.38 y=0
zone back2 treasure x=0.38 y=0
link p1 p2 weak
link p1 north normal
link p2 north normal
link p1 south weak
link p2 south weak
link p1 back1 weak
link p2 back2 weak
)tpl"},
    {"chain.txt",
     R"tpl(# Chain: the starts in a line, each two neighbours sharing a zone between them.
)tpl"
     R"tpl(
name Chain
players 2
sizes small-medium

zone p1 start x=-0.34 y=0.2
zone p2 start x=0.34 y=-0.2
zone link1 treasure rich town x=0 y=0
link p1 link1 normal
link p2 link1 normal
)tpl"
     R"tpl(
name Chain
players 2
sizes large-huge

zone p1 start x=-0.34 y=0.2
zone own1 treasure size=0.8 x=-0.34 y=-0.36
zone p2 start x=0.34 y=-0.2
zone own2 treasure size=0.8 x=0.34 y=0.36
zone link1 treasure rich town x=0 y=0
link p1 own1 weak
link p2 own2 weak
link p1 link1 normal
link p2 link1 normal
)tpl"
     R"tpl(
name Chain
players 3
sizes small-medium

zone p1 start x=-0.34 y=0.2
zone p2 start x=0 y=-0.2
zone p3 start x=0.34 y=0.2
zone link1 treasure rich town x=-0.17 y=0
zone link2 treasure x=0.17 y=0
link p1 link1 normal
link p2 link1 normal
link p2 link2 normal
link p3 link2 normal
)tpl"
     R"tpl(
name Chain
players 3
sizes large-huge

zone p1 start x=-0.34 y=0.2
zone own1 treasure size=0.8 x=-0.34 y=-0.36
zone p2 start x=0 y=-0.2
zone own2 treasure size=0.8 x=0 y=0.36
zone p3 start x=0.34 y=0.2
zone own3 treasure size=0.8 x=0.34 y=-0.36
zone link1 treasure rich town x=-0.17 y=0
zone link2 treasure x=0.17 y=0
link p1 own1 weak
link p2 own2 weak
link p3 own3 weak
link p1 link1 normal
link p2 link1 normal
link p2 link2 normal
link p3 link2 normal
)tpl"
     R"tpl(
name Chain
players 4
sizes small-medium

zone p1 start x=-0.34 y=0.2
zone p2 start x=-0.113 y=-0.2
zone p3 start x=0.113 y=0.2
zone p4 start x=0.34 y=-0.2
zone link1 treasure x=-0.227 y=0
zone link2 treasure rich town x=0 y=0
zone link3 treasure x=0.227 y=0
link p1 link1 normal
link p2 link1 normal
link p2 link2 normal
link p3 link2 normal
link p3 link3 normal
link p4 link3 normal
)tpl"
     R"tpl(
name Chain
players 4
sizes large-huge

zone p1 start x=-0.34 y=0.2
zone own1 treasure size=0.8 x=-0.34 y=-0.36
zone p2 start x=-0.113 y=-0.2
zone own2 treasure size=0.8 x=-0.113 y=0.36
zone p3 start x=0.113 y=0.2
zone own3 treasure size=0.8 x=0.113 y=-0.36
zone p4 start x=0.34 y=-0.2
zone own4 treasure size=0.8 x=0.34 y=0.36
zone link1 treasure x=-0.227 y=0
zone link2 treasure rich town x=0 y=0
zone link3 treasure x=0.227 y=0
link p1 own1 weak
link p2 own2 weak
link p3 own3 weak
link p4 own4 weak
link p1 link1 normal
link p2 link1 normal
link p2 link2 normal
link p3 link2 normal
link p3 link3 normal
link p4 link3 normal
)tpl"
     R"tpl(
name Chain
players 5
sizes small-medium

zone p1 start x=-0.34 y=0.2
zone p2 start x=-0.17 y=-0.2
zone p3 start x=0 y=0.2
zone p4 start x=0.17 y=-0.2
zone p5 start x=0.34 y=0.2
zone link1 treasure x=-0.255 y=0
zone link2 treasure rich town x=-0.085 y=0
zone link3 treasure x=0.085 y=0
zone link4 treasure x=0.255 y=0
link p1 link1 normal
link p2 link1 normal
link p2 link2 normal
link p3 link2 normal
link p3 link3 normal
link p4 link3 normal
link p4 link4 normal
link p5 link4 normal
)tpl"
     R"tpl(
name Chain
players 5
sizes large-huge

zone p1 start x=-0.34 y=0.2
zone own1 treasure size=0.8 x=-0.34 y=-0.36
zone p2 start x=-0.17 y=-0.2
zone own2 treasure size=0.8 x=-0.17 y=0.36
zone p3 start x=0 y=0.2
zone own3 treasure size=0.8 x=0 y=-0.36
zone p4 start x=0.17 y=-0.2
zone own4 treasure size=0.8 x=0.17 y=0.36
zone p5 start x=0.34 y=0.2
zone own5 treasure size=0.8 x=0.34 y=-0.36
zone link1 treasure x=-0.255 y=0
zone link2 treasure rich town x=-0.085 y=0
zone link3 treasure x=0.085 y=0
zone link4 treasure x=0.255 y=0
link p1 own1 weak
link p2 own2 weak
link p3 own3 weak
link p4 own4 weak
link p5 own5 weak
link p1 link1 normal
link p2 link1 normal
link p2 link2 normal
link p3 link2 normal
link p3 link3 normal
link p4 link3 normal
link p4 link4 normal
link p5 link4 normal
)tpl"
     R"tpl(
name Chain
players 6
sizes small-medium

zone p1 start x=-0.34 y=0.2
zone p2 start x=-0.204 y=-0.2
zone p3 start x=-0.068 y=0.2
zone p4 start x=0.068 y=-0.2
zone p5 start x=0.204 y=0.2
zone p6 start x=0.34 y=-0.2
zone link1 treasure x=-0.272 y=0
zone link2 treasure x=-0.136 y=0
zone link3 treasure rich town x=0 y=0
zone link4 treasure x=0.136 y=0
zone link5 treasure x=0.272 y=0
link p1 link1 normal
link p2 link1 normal
link p2 link2 normal
link p3 link2 normal
link p3 link3 normal
link p4 link3 normal
link p4 link4 normal
link p5 link4 normal
link p5 link5 normal
link p6 link5 normal
)tpl"
     R"tpl(
name Chain
players 6
sizes large-huge

zone p1 start x=-0.34 y=0.2
zone own1 treasure size=0.8 x=-0.34 y=-0.36
zone p2 start x=-0.204 y=-0.2
zone own2 treasure size=0.8 x=-0.204 y=0.36
zone p3 start x=-0.068 y=0.2
zone own3 treasure size=0.8 x=-0.068 y=-0.36
zone p4 start x=0.068 y=-0.2
zone own4 treasure size=0.8 x=0.068 y=0.36
zone p5 start x=0.204 y=0.2
zone own5 treasure size=0.8 x=0.204 y=-0.36
zone p6 start x=0.34 y=-0.2
zone own6 treasure size=0.8 x=0.34 y=0.36
zone link1 treasure x=-0.272 y=0
zone link2 treasure x=-0.136 y=0
zone link3 treasure rich town x=0 y=0
zone link4 treasure x=0.136 y=0
zone link5 treasure x=0.272 y=0
link p1 own1 weak
link p2 own2 weak
link p3 own3 weak
link p4 own4 weak
link p5 own5 weak
link p6 own6 weak
link p1 link1 normal
link p2 link1 normal
link p2 link2 normal
link p3 link2 normal
link p3 link3 normal
link p4 link3 normal
link p4 link4 normal
link p5 link4 normal
link p5 link5 normal
link p6 link5 normal
)tpl"},
    {"checkerboard.txt",
     R"tpl(# Checkerboard: a grid of zones, the starts in its corners, the riches in its middle.
)tpl"
     R"tpl(
name Checkerboard
players 2
sizes small-medium

zone p1 start x=-0.35 y=-0.35
zone square12 treasure x=0 y=-0.35
zone square13 treasure x=0.35 y=-0.35
zone square21 treasure x=-0.35 y=0
zone square22 treasure rich town x=0 y=0
zone square23 treasure x=0.35 y=0
zone square31 treasure x=-0.35 y=0.35
zone square32 treasure x=0 y=0.35
zone p2 start x=0.35 y=0.35
link p1 square12 strong
link p1 square21 normal
link square12 square13 normal
link square12 square22 strong
link square13 square23 normal
link square21 square22 normal
link square21 square31 strong
link square22 square23 strong
link square22 square32 normal
link square23 p2 strong
link square31 square32 strong
link square32 p2 normal
)tpl"
     R"tpl(
name Checkerboard
players 2
sizes large-huge

zone p1 start x=-0.35 y=-0.35
zone square12 treasure x=-0.117 y=-0.35
zone square13 treasure x=0.117 y=-0.35
zone square14 treasure x=0.35 y=-0.35
zone square21 treasure x=-0.35 y=-0.117
zone square22 treasure rich x=-0.117 y=-0.117
zone square23 treasure rich x=0.117 y=-0.117
zone square24 treasure x=0.35 y=-0.117
zone square31 treasure x=-0.35 y=0.117
zone square32 treasure rich x=-0.117 y=0.117
zone square33 treasure rich town x=0.117 y=0.117
zone square34 treasure x=0.35 y=0.117
zone square41 treasure x=-0.35 y=0.35
zone square42 treasure x=-0.117 y=0.35
zone square43 treasure x=0.117 y=0.35
zone p2 start x=0.35 y=0.35
link p1 square12 strong
link p1 square21 normal
link square12 square13 normal
link square12 square22 strong
link square13 square14 strong
link square13 square23 normal
link square14 square24 strong
link square21 square22 normal
link square21 square31 strong
link square22 square23 strong
link square22 square32 normal
link square23 square24 normal
link square23 square33 strong
link square24 square34 normal
link square31 square32 strong
link square31 square41 normal
link square32 square33 normal
link square32 square42 strong
link square33 square34 strong
link square33 square43 normal
link square34 p2 strong
link square41 square42 normal
link square42 square43 strong
link square43 p2 normal
)tpl"
     R"tpl(
name Checkerboard
players 3
sizes small-medium

zone p1 start x=-0.35 y=-0.35
zone square12 treasure x=0 y=-0.35
zone p3 start x=0.35 y=-0.35
zone square21 treasure x=-0.35 y=0
zone square22 treasure rich town x=0 y=0
zone square23 treasure x=0.35 y=0
zone square31 treasure x=-0.35 y=0.35
zone square32 treasure x=0 y=0.35
zone p2 start x=0.35 y=0.35
link p1 square12 strong
link p1 square21 normal
link square12 p3 normal
link square12 square22 strong
link p3 square23 normal
link square21 square22 normal
link square21 square31 strong
link square22 square23 strong
link square22 square32 normal
link square23 p2 strong
link square31 square32 strong
link square32 p2 normal
)tpl"
     R"tpl(
name Checkerboard
players 3
sizes large-huge

zone p1 start x=-0.35 y=-0.35
zone square12 treasure x=-0.117 y=-0.35
zone square13 treasure x=0.117 y=-0.35
zone p3 start x=0.35 y=-0.35
zone square21 treasure x=-0.35 y=-0.117
zone square22 treasure rich x=-0.117 y=-0.117
zone square23 treasure rich x=0.117 y=-0.117
zone square24 treasure x=0.35 y=-0.117
zone square31 treasure x=-0.35 y=0.117
zone square32 treasure rich x=-0.117 y=0.117
zone square33 treasure rich town x=0.117 y=0.117
zone square34 treasure x=0.35 y=0.117
zone square41 treasure x=-0.35 y=0.35
zone square42 treasure x=-0.117 y=0.35
zone square43 treasure x=0.117 y=0.35
zone p2 start x=0.35 y=0.35
link p1 square12 strong
link p1 square21 normal
link square12 square13 normal
link square12 square22 strong
link square13 p3 strong
link square13 square23 normal
link p3 square24 strong
link square21 square22 normal
link square21 square31 strong
link square22 square23 strong
link square22 square32 normal
link square23 square24 normal
link square23 square33 strong
link square24 square34 normal
link square31 square32 strong
link square31 square41 normal
link square32 square33 normal
link square32 square42 strong
link square33 square34 strong
link square33 square43 normal
link square34 p2 strong
link square41 square42 normal
link square42 square43 strong
link square43 p2 normal
)tpl"
     R"tpl(
name Checkerboard
players 4
sizes small-medium

zone p1 start x=-0.35 y=-0.35
zone square12 treasure x=0 y=-0.35
zone p3 start x=0.35 y=-0.35
zone square21 treasure x=-0.35 y=0
zone square22 treasure rich town x=0 y=0
zone square23 treasure x=0.35 y=0
zone p4 start x=-0.35 y=0.35
zone square32 treasure x=0 y=0.35
zone p2 start x=0.35 y=0.35
link p1 square12 strong
link p1 square21 normal
link square12 p3 normal
link square12 square22 strong
link p3 square23 normal
link square21 square22 normal
link square21 p4 strong
link square22 square23 strong
link square22 square32 normal
link square23 p2 strong
link p4 square32 strong
link square32 p2 normal
)tpl"
     R"tpl(
name Checkerboard
players 4
sizes large-huge

zone p1 start x=-0.35 y=-0.35
zone square12 treasure x=-0.117 y=-0.35
zone square13 treasure x=0.117 y=-0.35
zone p3 start x=0.35 y=-0.35
zone square21 treasure x=-0.35 y=-0.117
zone square22 treasure rich x=-0.117 y=-0.117
zone square23 treasure rich x=0.117 y=-0.117
zone square24 treasure x=0.35 y=-0.117
zone square31 treasure x=-0.35 y=0.117
zone square32 treasure rich x=-0.117 y=0.117
zone square33 treasure rich town x=0.117 y=0.117
zone square34 treasure x=0.35 y=0.117
zone p4 start x=-0.35 y=0.35
zone square42 treasure x=-0.117 y=0.35
zone square43 treasure x=0.117 y=0.35
zone p2 start x=0.35 y=0.35
link p1 square12 strong
link p1 square21 normal
link square12 square13 normal
link square12 square22 strong
link square13 p3 strong
link square13 square23 normal
link p3 square24 strong
link square21 square22 normal
link square21 square31 strong
link square22 square23 strong
link square22 square32 normal
link square23 square24 normal
link square23 square33 strong
link square24 square34 normal
link square31 square32 strong
link square31 p4 normal
link square32 square33 normal
link square32 square42 strong
link square33 square34 strong
link square33 square43 normal
link square34 p2 strong
link p4 square42 normal
link square42 square43 strong
link square43 p2 normal
)tpl"},
    {"clash.txt",
     R"tpl(# Clash: the starts close round the middle, the riches out by the map's edges.
)tpl"
     R"tpl(
name Clash
players 2
sizes small-medium

zone p1 start size=1.1 r=0.17 turn=0
zone p2 start size=1.1 r=0.17 turn=1/2
zone out1 treasure rich r=0.37 turn=0
zone gap1 treasure r=0.34 turn=1/4
zone out2 treasure rich r=0.37 turn=1/2
zone gap2 treasure r=0.34 turn=3/4
link p1 out1 strong
link gap1 out1 normal
link gap1 out2 normal
link p1 p2 normal
link p2 out2 strong
link gap2 out2 normal
)tpl"
     R"tpl(
name Clash
players 2
sizes large-huge

zone p1 start size=1.1 r=0.17 turn=0
zone p2 start size=1.1 r=0.17 turn=1/2
zone out1 treasure rich town r=0.37 turn=0
zone gap1 treasure r=0.34 turn=1/4
zone out2 treasure rich r=0.37 turn=1/2
zone gap2 treasure r=0.34 turn=3/4
link p1 out1 strong
link gap1 out1 normal
link gap1 out2 normal
link p1 p2 normal
link p2 out2 strong
link gap2 out2 normal
)tpl"
     R"tpl(
name Clash
players 3
sizes small-medium

zone p1 start size=1.1 r=0.17 turn=0
zone p2 start size=1.1 r=0.17 turn=1/3
zone p3 start size=1.1 r=0.17 turn=2/3
zone out1 treasure rich r=0.37 turn=0
zone gap1 treasure r=0.34 turn=1/6
zone out2 treasure rich r=0.37 turn=1/3
zone gap2 treasure r=0.34 turn=1/2
zone out3 treasure rich r=0.37 turn=2/3
zone gap3 treasure r=0.34 turn=5/6
link p1 out1 strong
link gap1 out1 normal
link gap1 out2 normal
link p1 p2 normal
link p2 out2 strong
link gap2 out2 normal
link gap2 out3 normal
link p2 p3 normal
link p3 out3 strong
link gap3 out3 normal
link gap3 out1 normal
link p3 p1 normal
)tpl"
     R"tpl(
name Clash
players 3
sizes large-huge

zone p1 start size=1.1 r=0.17 turn=0
zone p2 start size=1.1 r=0.17 turn=1/3
zone p3 start size=1.1 r=0.17 turn=2/3
zone out1 treasure rich town r=0.37 turn=0
zone gap1 treasure r=0.34 turn=1/6
zone out2 treasure rich r=0.37 turn=1/3
zone gap2 treasure r=0.34 turn=1/2
zone out3 treasure rich town r=0.37 turn=2/3
zone gap3 treasure r=0.34 turn=5/6
link p1 out1 strong
link gap1 out1 normal
link gap1 out2 normal
link p1 p2 normal
link p2 out2 strong
link gap2 out2 normal
link gap2 out3 normal
link p2 p3 normal
link p3 out3 strong
link gap3 out3 normal
link gap3 out1 normal
link p3 p1 normal
)tpl"
     R"tpl(
name Clash
players 4
sizes small-medium

zone p1 start size=1.1 r=0.17 turn=0
zone p2 start size=1.1 r=0.17 turn=1/4
zone p3 start size=1.1 r=0.17 turn=1/2
zone p4 start size=1.1 r=0.17 turn=3/4
zone out1 treasure rich r=0.37 turn=0
zone gap1 treasure r=0.34 turn=1/8
zone out2 treasure rich r=0.37 turn=1/4
zone gap2 treasure r=0.34 turn=3/8
zone out3 treasure rich r=0.37 turn=1/2
zone gap3 treasure r=0.34 turn=5/8
zone out4 treasure rich r=0.37 turn=3/4
zone gap4 treasure r=0.34 turn=7/8
link p1 out1 strong
link gap1 out1 normal
link gap1 out2 normal
link p1 p2 normal
link p2 out2 strong
link gap2 out2 normal
link gap2 out3 normal
link p2 p3 normal
link p3 out3 strong
link gap3 out3 normal
link gap3 out4 normal
link p3 p4 normal
link p4 out4 strong
link gap4 out4 normal
link gap4 out1 normal
link p4 p1 normal
)tpl"
     R"tpl(
name Clash
players 4
sizes large-huge

zone p1 start size=1.1 r=0.17 turn=0
zone p2 start size=1.1 r=0.17 turn=1/4
zone p3 start size=1.1 r=0.17 turn=1/2
zone p4 start size=1.1 r=0.17 turn=3/4
zone out1 treasure rich town r=0.37 turn=0
zone gap1 treasure r=0.34 turn=1/8
zone out2 treasure rich r=0.37 turn=1/4
zone gap2 treasure r=0.34 turn=3/8
zone out3 treasure rich town r=0.37 turn=1/2
zone gap3 treasure r=0.34 turn=5/8
zone out4 treasure rich r=0.37 turn=3/4
zone gap4 treasure r=0.34 turn=7/8
link p1 out1 strong
link gap1 out1 normal
link gap1 out2 normal
link p1 p2 normal
link p2 out2 strong
link gap2 out2 normal
link gap2 out3 normal
link p2 p3 normal
link p3 out3 strong
link gap3 out3 normal
link gap3 out4 normal
link p3 p4 normal
link p4 out4 strong
link gap4 out4 normal
link gap4 out1 normal
link p4 p1 normal
)tpl"
     R"tpl(
name Clash
players 5
sizes medium

zone p1 start size=1.1 r=0.17 turn=0
zone p2 start size=1.1 r=0.17 turn=1/5
zone p3 start size=1.1 r=0.17 turn=2/5
zone p4 start size=1.1 r=0.17 turn=3/5
zone p5 start size=1.1 r=0.17 turn=4/5
zone out1 treasure rich r=0.37 turn=0
zone gap1 treasure r=0.34 turn=1/10
zone out2 treasure rich r=0.37 turn=1/5
zone gap2 treasure r=0.34 turn=3/10
zone out3 treasure rich r=0.37 turn=2/5
zone gap3 treasure r=0.34 turn=1/2
zone out4 treasure rich r=0.37 turn=3/5
zone gap4 treasure r=0.34 turn=7/10
zone out5 treasure rich r=0.37 turn=4/5
zone gap5 treasure r=0.34 turn=9/10
link p1 out1 strong
link gap1 out1 normal
link gap1 out2 normal
link p1 p2 normal
link p2 out2 strong
link gap2 out2 normal
link gap2 out3 normal
link p2 p3 normal
link p3 out3 strong
link gap3 out3 normal
link gap3 out4 normal
link p3 p4 normal
link p4 out4 strong
link gap4 out4 normal
link gap4 out5 normal
link p4 p5 normal
link p5 out5 strong
link gap5 out5 normal
link gap5 out1 normal
link p5 p1 normal
)tpl"
     R"tpl(
name Clash
players 5
sizes large-huge

zone p1 start size=1.1 r=0.17 turn=0
zone p2 start size=1.1 r=0.17 turn=1/5
zone p3 start size=1.1 r=0.17 turn=2/5
zone p4 start size=1.1 r=0.17 turn=3/5
zone p5 start size=1.1 r=0.17 turn=4/5
zone out1 treasure rich town r=0.37 turn=0
zone gap1 treasure r=0.34 turn=1/10
zone out2 treasure rich r=0.37 turn=1/5
zone gap2 treasure r=0.34 turn=3/10
zone out3 treasure rich town r=0.37 turn=2/5
zone gap3 treasure r=0.34 turn=1/2
zone out4 treasure rich r=0.37 turn=3/5
zone gap4 treasure r=0.34 turn=7/10
zone out5 treasure rich town r=0.37 turn=4/5
zone gap5 treasure r=0.34 turn=9/10
link p1 out1 strong
link gap1 out1 normal
link gap1 out2 normal
link p1 p2 normal
link p2 out2 strong
link gap2 out2 normal
link gap2 out3 normal
link p2 p3 normal
link p3 out3 strong
link gap3 out3 normal
link gap3 out4 normal
link p3 p4 normal
link p4 out4 strong
link gap4 out4 normal
link gap4 out5 normal
link p4 p5 normal
link p5 out5 strong
link gap5 out5 normal
link gap5 out1 normal
link p5 p1 normal
)tpl"
     R"tpl(
name Clash
players 6
sizes medium

zone p1 start size=1.1 r=0.17 turn=0
zone p2 start size=1.1 r=0.17 turn=1/6
zone p3 start size=1.1 r=0.17 turn=1/3
zone p4 start size=1.1 r=0.17 turn=1/2
zone p5 start size=1.1 r=0.17 turn=2/3
zone p6 start size=1.1 r=0.17 turn=5/6
zone out1 treasure rich r=0.37 turn=0
zone gap1 treasure r=0.34 turn=1/12
zone out2 treasure rich r=0.37 turn=1/6
zone gap2 treasure r=0.34 turn=1/4
zone out3 treasure rich r=0.37 turn=1/3
zone gap3 treasure r=0.34 turn=5/12
zone out4 treasure rich r=0.37 turn=1/2
zone gap4 treasure r=0.34 turn=7/12
zone out5 treasure rich r=0.37 turn=2/3
zone gap5 treasure r=0.34 turn=3/4
zone out6 treasure rich r=0.37 turn=5/6
zone gap6 treasure r=0.34 turn=11/12
link p1 out1 strong
link gap1 out1 normal
link gap1 out2 normal
link p1 p2 normal
link p2 out2 strong
link gap2 out2 normal
link gap2 out3 normal
link p2 p3 normal
link p3 out3 strong
link gap3 out3 normal
link gap3 out4 normal
link p3 p4 normal
link p4 out4 strong
link gap4 out4 normal
link gap4 out5 normal
link p4 p5 normal
link p5 out5 strong
link gap5 out5 normal
link gap5 out6 normal
link p5 p6 normal
link p6 out6 strong
link gap6 out6 normal
link gap6 out1 normal
link p6 p1 normal
)tpl"
     R"tpl(
name Clash
players 6
sizes large-huge

zone p1 start size=1.1 r=0.17 turn=0
zone p2 start size=1.1 r=0.17 turn=1/6
zone p3 start size=1.1 r=0.17 turn=1/3
zone p4 start size=1.1 r=0.17 turn=1/2
zone p5 start size=1.1 r=0.17 turn=2/3
zone p6 start size=1.1 r=0.17 turn=5/6
zone out1 treasure rich town r=0.37 turn=0
zone gap1 treasure r=0.34 turn=1/12
zone out2 treasure rich r=0.37 turn=1/6
zone gap2 treasure r=0.34 turn=1/4
zone out3 treasure rich town r=0.37 turn=1/3
zone gap3 treasure r=0.34 turn=5/12
zone out4 treasure rich r=0.37 turn=1/2
zone gap4 treasure r=0.34 turn=7/12
zone out5 treasure rich town r=0.37 turn=2/3
zone gap5 treasure r=0.34 turn=3/4
zone out6 treasure rich r=0.37 turn=5/6
zone gap6 treasure r=0.34 turn=11/12
link p1 out1 strong
link gap1 out1 normal
link gap1 out2 normal
link p1 p2 normal
link p2 out2 strong
link gap2 out2 normal
link gap2 out3 normal
link p2 p3 normal
link p3 out3 strong
link gap3 out3 normal
link gap3 out4 normal
link p3 p4 normal
link p4 out4 strong
link gap4 out4 normal
link gap4 out5 normal
link p4 p5 normal
link p5 out5 strong
link gap5 out5 normal
link gap5 out6 normal
link p5 p6 normal
link p6 out6 strong
link gap6 out6 normal
link gap6 out1 normal
link p6 p1 normal
)tpl"},
    {"corridor.txt",
     R"tpl(# Corridor: two players at the ends of a line of zones, pockets on its sides, the richest
# (and a town) in the middle.
)tpl"
     R"tpl(
name Corridor
players 2
sizes small-medium

zone p1 start size=1.2 x=-0.36 y=0
zone way1 treasure x=-0.18 y=0
zone way2 treasure rich town x=0 y=0
zone way3 treasure x=0.18 y=0
zone p2 start size=1.2 x=0.36 y=0
zone pocket1n treasure rich size=0.8 x=-0.18 y=-0.28
zone pocket1s treasure rich size=0.8 x=-0.18 y=0.28
zone pocket2n treasure rich size=0.8 x=0 y=-0.28
zone pocket2s treasure rich size=0.8 x=0 y=0.28
zone pocket3n treasure rich size=0.8 x=0.18 y=-0.28
zone pocket3s treasure rich size=0.8 x=0.18 y=0.28
link p1 way1 normal
link way1 way2 strong
link way2 way3 strong
link way3 p2 normal
link way1 pocket1n strong
link way1 pocket1s strong
link way2 pocket2n strong
link way2 pocket2s strong
link way3 pocket3n strong
link way3 pocket3s strong
)tpl"
     R"tpl(
name Corridor
players 2
sizes large-huge

zone p1 start size=1.2 x=-0.38 y=0
zone way1 treasure x=-0.25 y=0
zone way2 treasure x=-0.12 y=0
zone way3 treasure rich town x=0 y=0
zone way4 treasure x=0.12 y=0
zone way5 treasure x=0.25 y=0
zone p2 start size=1.2 x=0.38 y=0
zone pocket1n treasure size=0.8 x=-0.25 y=-0.28
zone pocket1s treasure size=0.8 x=-0.25 y=0.28
zone pocket2n treasure rich size=0.8 x=-0.12 y=-0.28
zone pocket2s treasure rich size=0.8 x=-0.12 y=0.28
zone pocket3n treasure rich size=0.8 x=0 y=-0.28
zone pocket3s treasure rich size=0.8 x=0 y=0.28
zone pocket4n treasure rich size=0.8 x=0.12 y=-0.28
zone pocket4s treasure rich size=0.8 x=0.12 y=0.28
zone pocket5n treasure size=0.8 x=0.25 y=-0.28
zone pocket5s treasure size=0.8 x=0.25 y=0.28
link p1 way1 normal
link way1 way2 normal
link way2 way3 strong
link way3 way4 strong
link way4 way5 normal
link way5 p2 normal
link way1 pocket1n weak
link way1 pocket1s weak
link way2 pocket2n strong
link way2 pocket2s strong
link way3 pocket3n strong
link way3 pocket3s strong
link way4 pocket4n strong
link way4 pocket4s strong
link way5 pocket5n weak
link way5 pocket5s weak
)tpl"},
    {"cross.txt",
     R"tpl(# Cross: four corners round a rich middle with a town; each start's own zone on the way in,
# the own zones linked in a ring; on large maps rich edges between the corners.
)tpl"
     R"tpl(
name Cross
players 2-4
sizes small-medium

zone middle treasure rich town size=1.4 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=1/8
zone p2 start size=1.2 r=0.37 turn=5/8
zone p3 start size=1.2 r=0.37 turn=3/8
zone p4 start size=1.2 r=0.37 turn=7/8
zone own1 treasure r=0.2 turn=1/8
zone own2 treasure r=0.2 turn=3/8
zone own3 treasure r=0.2 turn=5/8
zone own4 treasure r=0.2 turn=7/8
link p1 own1
link own1 middle deadly
link p3 own2
link own2 middle deadly
link p2 own3
link own3 middle deadly
link p4 own4
link own4 middle deadly
link own1 own2 normal
link own2 own3 normal
link own3 own4 normal
link own4 own1 normal
)tpl"
     R"tpl(
name Cross
players 2-4
sizes large-huge

zone middle treasure rich town size=1.4 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=1/8
zone p2 start size=1.2 r=0.37 turn=5/8
zone p3 start size=1.2 r=0.37 turn=3/8
zone p4 start size=1.2 r=0.37 turn=7/8
zone own1 treasure r=0.2 turn=1/8
zone own2 treasure r=0.2 turn=3/8
zone own3 treasure r=0.2 turn=5/8
zone own4 treasure r=0.2 turn=7/8
zone edge1 treasure rich r=0.38 turn=1/4
zone edge2 treasure rich r=0.38 turn=1/2
zone edge3 treasure rich r=0.38 turn=3/4
zone edge4 treasure rich r=0.38 turn=0
link p1 own1
link own1 middle deadly
link p3 own2
link own2 middle deadly
link p2 own3
link own3 middle deadly
link p4 own4
link own4 middle deadly
link own1 own2 normal
link own2 own3 normal
link own3 own4 normal
link own4 own1 normal
link p1 edge1 strong
link p3 edge1 strong
link p3 edge2 strong
link p2 edge2 strong
link p2 edge3 strong
link p4 edge3 strong
link p4 edge4 strong
link p1 edge4 strong
)tpl"},
    {"crown.txt",
     R"tpl(# Crown: the starts along the bottom, a band between them, the rich points along the top.
)tpl"
     R"tpl(
name Crown
players 2
sizes small-medium

zone p1 start x=-0.33 y=0.3
zone band1 treasure size=0.8 x=-0.33 y=0.02
zone p2 start x=0.33 y=0.3
zone band2 treasure size=0.8 x=0.33 y=0.02
zone point1 treasure rich town x=0 y=-0.3
link p1 band1 weak
link p2 band2 weak
link band1 point1 strong
link band2 point1 strong
link band1 band2 normal
)tpl"
     R"tpl(
name Crown
players 2
sizes large-huge

zone p1 start x=-0.33 y=0.3
zone band1 treasure size=0.8 x=-0.33 y=0.02
zone p2 start x=0.33 y=0.3
zone band2 treasure size=0.8 x=0.33 y=0.02
zone point1 treasure rich town x=0 y=-0.3
link p1 band1 weak
link p2 band2 weak
link band1 point1 strong
link band2 point1 strong
link band1 band2 normal
)tpl"
     R"tpl(
name Crown
players 3
sizes small-medium

zone p1 start x=-0.33 y=0.3
zone band1 treasure size=0.8 x=-0.33 y=0.02
zone p2 start x=0 y=0.3
zone band2 treasure size=0.8 x=0 y=0.02
zone p3 start x=0.33 y=0.3
zone band3 treasure size=0.8 x=0.33 y=0.02
zone point1 treasure rich x=-0.165 y=-0.3
zone point2 treasure rich town x=0.165 y=-0.3
link p1 band1 weak
link p2 band2 weak
link p3 band3 weak
link band1 point1 strong
link band2 point1 strong
link band2 point2 strong
link band3 point2 strong
link band1 band2 normal
link band2 band3 normal
)tpl"
     R"tpl(
name Crown
players 3
sizes large-huge

zone p1 start x=-0.33 y=0.3
zone band1 treasure size=0.8 x=-0.33 y=0.02
zone p2 start x=0 y=0.3
zone band2 treasure size=0.8 x=0 y=0.02
zone p3 start x=0.33 y=0.3
zone band3 treasure size=0.8 x=0.33 y=0.02
zone point1 treasure rich x=-0.165 y=-0.3
zone point2 treasure rich town x=0.165 y=-0.3
link p1 band1 weak
link p2 band2 weak
link p3 band3 weak
link band1 point1 strong
link band2 point1 strong
link band2 point2 strong
link band3 point2 strong
link band1 band2 normal
link band2 band3 normal
)tpl"
     R"tpl(
name Crown
players 4
sizes small-medium

zone p1 start x=-0.33 y=0.3
zone band1 treasure size=0.8 x=-0.33 y=0.02
zone p2 start x=-0.11 y=0.3
zone band2 treasure size=0.8 x=-0.11 y=0.02
zone p3 start x=0.11 y=0.3
zone band3 treasure size=0.8 x=0.11 y=0.02
zone p4 start x=0.33 y=0.3
zone band4 treasure size=0.8 x=0.33 y=0.02
zone point1 treasure rich x=-0.22 y=-0.3
zone point2 treasure rich town x=0 y=-0.3
zone point3 treasure rich x=0.22 y=-0.3
link p1 band1 weak
link p2 band2 weak
link p3 band3 weak
link p4 band4 weak
link band1 point1 strong
link band2 point1 strong
link band2 point2 strong
link band3 point2 strong
link band3 point3 strong
link band4 point3 strong
link band1 band2 normal
link band2 band3 normal
link band3 band4 normal
)tpl"
     R"tpl(
name Crown
players 4
sizes large-huge

zone p1 start x=-0.33 y=0.3
zone band1 treasure size=0.8 x=-0.33 y=0.02
zone p2 start x=-0.11 y=0.3
zone band2 treasure size=0.8 x=-0.11 y=0.02
zone p3 start x=0.11 y=0.3
zone band3 treasure size=0.8 x=0.11 y=0.02
zone p4 start x=0.33 y=0.3
zone band4 treasure size=0.8 x=0.33 y=0.02
zone point1 treasure rich x=-0.22 y=-0.3
zone point2 treasure rich town x=0 y=-0.3
zone point3 treasure rich x=0.22 y=-0.3
link p1 band1 weak
link p2 band2 weak
link p3 band3 weak
link p4 band4 weak
link band1 point1 strong
link band2 point1 strong
link band2 point2 strong
link band3 point2 strong
link band3 point3 strong
link band4 point3 strong
link band1 band2 normal
link band2 band3 normal
link band3 band4 normal
)tpl"
     R"tpl(
name Crown
players 5
sizes medium

zone p1 start x=-0.33 y=0.3
zone band1 treasure size=0.8 x=-0.33 y=0.02
zone p2 start x=-0.165 y=0.3
zone band2 treasure size=0.8 x=-0.165 y=0.02
zone p3 start x=0 y=0.3
zone band3 treasure size=0.8 x=0 y=0.02
zone p4 start x=0.165 y=0.3
zone band4 treasure size=0.8 x=0.165 y=0.02
zone p5 start x=0.33 y=0.3
zone band5 treasure size=0.8 x=0.33 y=0.02
zone point1 treasure rich x=-0.247 y=-0.3
zone point2 treasure rich x=-0.083 y=-0.3
zone point3 treasure rich town x=0.083 y=-0.3
zone point4 treasure rich x=0.247 y=-0.3
link p1 band1 weak
link p2 band2 weak
link p3 band3 weak
link p4 band4 weak
link p5 band5 weak
link band1 point1 strong
link band2 point1 strong
link band2 point2 strong
link band3 point2 strong
link band3 point3 strong
link band4 point3 strong
link band4 point4 strong
link band5 point4 strong
link band1 band2 normal
link band2 band3 normal
link band3 band4 normal
link band4 band5 normal
)tpl"
     R"tpl(
name Crown
players 5
sizes large-huge

zone p1 start x=-0.33 y=0.3
zone band1 treasure size=0.8 x=-0.33 y=0.02
zone p2 start x=-0.165 y=0.3
zone band2 treasure size=0.8 x=-0.165 y=0.02
zone p3 start x=0 y=0.3
zone band3 treasure size=0.8 x=0 y=0.02
zone p4 start x=0.165 y=0.3
zone band4 treasure size=0.8 x=0.165 y=0.02
zone p5 start x=0.33 y=0.3
zone band5 treasure size=0.8 x=0.33 y=0.02
zone point1 treasure rich x=-0.247 y=-0.3
zone point2 treasure rich x=-0.083 y=-0.3
zone point3 treasure rich town x=0.083 y=-0.3
zone point4 treasure rich x=0.247 y=-0.3
link p1 band1 weak
link p2 band2 weak
link p3 band3 weak
link p4 band4 weak
link p5 band5 weak
link band1 point1 strong
link band2 point1 strong
link band2 point2 strong
link band3 point2 strong
link band3 point3 strong
link band4 point3 strong
link band4 point4 strong
link band5 point4 strong
link band1 band2 normal
link band2 band3 normal
link band3 band4 normal
link band4 band5 normal
)tpl"},
    {"diamond.txt",
     R"tpl(# Diamond: the starts on the sides' middles, corner zones between them, a rich middle.
)tpl"
     R"tpl(
name Diamond
players 2-4
sizes small-medium

zone middle treasure rich town size=1.3 x=0 y=0
zone p1 start size=1.1 x=0 y=-0.36
zone p2 start size=1.1 x=0 y=0.36
zone p3 start size=1.1 x=-0.36 y=0
zone p4 start size=1.1 x=0.36 y=0
zone corner1 treasure x=-0.22 y=-0.22
zone corner2 treasure x=0.22 y=-0.22
zone corner3 treasure x=0.22 y=0.22
zone corner4 treasure x=-0.22 y=0.22
link p1 corner1 weak
link p1 corner2 weak
link p2 corner3 weak
link p2 corner4 weak
link p3 corner1 weak
link p3 corner4 weak
link p4 corner2 weak
link p4 corner3 weak
link corner1 middle strong
link corner2 middle strong
link corner3 middle strong
link corner4 middle strong
)tpl"
     R"tpl(
name Diamond
players 2-4
sizes large-huge

zone middle treasure rich town size=1.3 x=0 y=0
zone p1 start size=1.1 x=0 y=-0.36
zone p2 start size=1.1 x=0 y=0.36
zone p3 start size=1.1 x=-0.36 y=0
zone p4 start size=1.1 x=0.36 y=0
zone corner1 treasure rich x=-0.22 y=-0.22
zone corner2 treasure rich x=0.22 y=-0.22
zone corner3 treasure rich x=0.22 y=0.22
zone corner4 treasure rich x=-0.22 y=0.22
zone pocket1 treasure size=0.7 x=0.14 y=-0.38
zone pocket2 treasure size=0.7 x=0.14 y=0.38
zone pocket3 treasure size=0.7 x=-0.38 y=0.14
zone pocket4 treasure size=0.7 x=0.38 y=0.14
link p1 corner1 normal
link p1 corner2 normal
link p2 corner3 normal
link p2 corner4 normal
link p3 corner1 normal
link p3 corner4 normal
link p4 corner2 normal
link p4 corner3 normal
link corner1 middle strong
link corner2 middle strong
link corner3 middle strong
link corner4 middle strong
link p1 pocket1 weak
link p2 pocket2 weak
link p3 pocket3 weak
link p4 pocket4 weak
)tpl"},
    {"duel.txt",
     R"tpl(# Duel: two players, mirrored halves; two treasure zones in front of each start, a rich middle
# with a town between them, rich edges on medium maps and up, corners on large ones.
)tpl"
     R"tpl(
name Duel
players 2
sizes small

zone middle1 treasure rich town x=0 y=0
zone p1 start size=1.2 x=-0.34 y=0
zone side1 treasure x=-0.18 y=-0.24
zone side2 treasure x=-0.18 y=0.24
zone p2 start size=1.2 x=0.34 y=0
zone side3 treasure x=0.18 y=-0.24
zone side4 treasure x=0.18 y=0.24
link p1 side1
link p1 side2
link side1 middle1 strong
link side2 middle1 strong
link p2 side3
link p2 side4
link side3 middle1 strong
link side4 middle1 strong
)tpl"
     R"tpl(
name Duel
players 2
sizes medium

zone middle1 treasure rich town size=1.3 x=0 y=0
zone p1 start size=1.2 x=-0.34 y=0
zone side1 treasure x=-0.18 y=-0.24
zone side2 treasure x=-0.18 y=0.24
zone p2 start size=1.2 x=0.34 y=0
zone side3 treasure x=0.18 y=-0.24
zone side4 treasure x=0.18 y=0.24
zone edge1 treasure rich x=0 y=-0.36
zone edge2 treasure rich x=0 y=0.36
link p1 side1
link p1 side2
link side1 middle1 strong
link side2 middle1 strong
link p2 side3
link p2 side4
link side3 middle1 strong
link side4 middle1 strong
link side1 edge1 deadly
link side2 edge2 deadly
link side3 edge1 deadly
link side4 edge2 deadly
)tpl"
     R"tpl(
name Duel
players 2
sizes large-huge

zone middle1 treasure rich town size=1.3 x=0 y=0
zone p1 start size=1.2 x=-0.34 y=0
zone side1 treasure x=-0.18 y=-0.24
zone side2 treasure x=-0.18 y=0.24
zone corner1 treasure size=0.9 x=-0.36 y=-0.34
zone corner2 treasure size=0.9 x=-0.36 y=0.34
zone p2 start size=1.2 x=0.34 y=0
zone side3 treasure x=0.18 y=-0.24
zone side4 treasure x=0.18 y=0.24
zone corner3 treasure size=0.9 x=0.36 y=-0.34
zone corner4 treasure size=0.9 x=0.36 y=0.34
zone edge1 treasure rich x=0 y=-0.36
zone edge2 treasure rich x=0 y=0.36
link p1 side1
link p1 side2
link side1 middle1 strong
link side2 middle1 strong
link side1 corner1
link side2 corner2
link p2 side3
link p2 side4
link side3 middle1 strong
link side4 middle1 strong
link side3 corner3
link side4 corner4
link side1 edge1 deadly
link side2 edge2 deadly
link side3 edge1 deadly
link side4 edge2 deadly
)tpl"},
    {"fortress.txt",
     R"tpl(# Fortress: each start behind one strongly guarded gate and its own yard; a wide land
# outside (and a rich middle for 3 players and more).
)tpl"
     R"tpl(
name Fortress
players 2
sizes small-medium

zone p1 start size=1.2 r=0.36 turn=0
zone yard1 treasure size=0.8 r=0.38 turn=1/4
zone gate1 treasure size=0.8 r=0.2 turn=0
zone field1 treasure r=0.22 turn=1/4
zone p2 start size=1.2 r=0.36 turn=1/2
zone yard2 treasure size=0.8 r=0.38 turn=3/4
zone gate2 treasure size=0.8 r=0.2 turn=1/2
zone field2 treasure r=0.22 turn=3/4
link p1 yard1 weak
link p1 gate1 strong
link gate1 field1 normal
link p2 yard2 weak
link p2 gate2 strong
link gate2 field2 normal
link field1 gate2 normal
)tpl"
     R"tpl(
name Fortress
players 2
sizes large-huge

zone p1 start size=1.2 r=0.36 turn=0
zone yard1 treasure size=0.8 r=0.38 turn=1/4
zone gate1 treasure size=0.8 r=0.2 turn=0
zone field1 treasure rich r=0.22 turn=1/4
zone p2 start size=1.2 r=0.36 turn=1/2
zone yard2 treasure size=0.8 r=0.38 turn=3/4
zone gate2 treasure size=0.8 r=0.2 turn=1/2
zone field2 treasure rich r=0.22 turn=3/4
link p1 yard1 weak
link p1 gate1 strong
link gate1 field1 normal
link p2 yard2 weak
link p2 gate2 strong
link gate2 field2 normal
link field1 gate2 normal
)tpl"
     R"tpl(
name Fortress
players 3
sizes small-medium

zone p1 start size=1.2 r=0.36 turn=0
zone yard1 treasure size=0.8 r=0.38 turn=1/6
zone gate1 treasure size=0.8 r=0.2 turn=0
zone field1 treasure r=0.22 turn=1/6
zone p2 start size=1.2 r=0.36 turn=1/3
zone yard2 treasure size=0.8 r=0.38 turn=1/2
zone gate2 treasure size=0.8 r=0.2 turn=1/3
zone field2 treasure r=0.22 turn=1/2
zone p3 start size=1.2 r=0.36 turn=2/3
zone yard3 treasure size=0.8 r=0.38 turn=5/6
zone gate3 treasure size=0.8 r=0.2 turn=2/3
zone field3 treasure r=0.22 turn=5/6
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 yard1 weak
link p1 gate1 strong
link gate1 field1 normal
link p2 yard2 weak
link p2 gate2 strong
link gate2 field2 normal
link p3 yard3 weak
link p3 gate3 strong
link gate3 field3 normal
link field1 gate2 normal
link field2 gate3 normal
link field3 gate1 normal
link field1 middle strong
link field2 middle strong
link field3 middle strong
)tpl"
     R"tpl(
name Fortress
players 3
sizes large-huge

zone p1 start size=1.2 r=0.36 turn=0
zone yard1 treasure size=0.8 r=0.38 turn=1/6
zone gate1 treasure size=0.8 r=0.2 turn=0
zone field1 treasure rich r=0.22 turn=1/6
zone p2 start size=1.2 r=0.36 turn=1/3
zone yard2 treasure size=0.8 r=0.38 turn=1/2
zone gate2 treasure size=0.8 r=0.2 turn=1/3
zone field2 treasure rich r=0.22 turn=1/2
zone p3 start size=1.2 r=0.36 turn=2/3
zone yard3 treasure size=0.8 r=0.38 turn=5/6
zone gate3 treasure size=0.8 r=0.2 turn=2/3
zone field3 treasure rich r=0.22 turn=5/6
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 yard1 weak
link p1 gate1 strong
link gate1 field1 normal
link p2 yard2 weak
link p2 gate2 strong
link gate2 field2 normal
link p3 yard3 weak
link p3 gate3 strong
link gate3 field3 normal
link field1 gate2 normal
link field2 gate3 normal
link field3 gate1 normal
link field1 middle strong
link field2 middle strong
link field3 middle strong
)tpl"
     R"tpl(
name Fortress
players 4
sizes medium

zone p1 start size=1.2 r=0.36 turn=0
zone yard1 treasure size=0.8 r=0.38 turn=1/8
zone gate1 treasure size=0.8 r=0.2 turn=0
zone field1 treasure r=0.22 turn=1/8
zone p2 start size=1.2 r=0.36 turn=1/4
zone yard2 treasure size=0.8 r=0.38 turn=3/8
zone gate2 treasure size=0.8 r=0.2 turn=1/4
zone field2 treasure r=0.22 turn=3/8
zone p3 start size=1.2 r=0.36 turn=1/2
zone yard3 treasure size=0.8 r=0.38 turn=5/8
zone gate3 treasure size=0.8 r=0.2 turn=1/2
zone field3 treasure r=0.22 turn=5/8
zone p4 start size=1.2 r=0.36 turn=3/4
zone yard4 treasure size=0.8 r=0.38 turn=7/8
zone gate4 treasure size=0.8 r=0.2 turn=3/4
zone field4 treasure r=0.22 turn=7/8
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 yard1 weak
link p1 gate1 strong
link gate1 field1 normal
link p2 yard2 weak
link p2 gate2 strong
link gate2 field2 normal
link p3 yard3 weak
link p3 gate3 strong
link gate3 field3 normal
link p4 yard4 weak
link p4 gate4 strong
link gate4 field4 normal
link field1 gate2 normal
link field2 gate3 normal
link field3 gate4 normal
link field4 gate1 normal
link field1 middle strong
link field2 middle strong
link field3 middle strong
link field4 middle strong
)tpl"
     R"tpl(
name Fortress
players 4
sizes large-huge

zone p1 start size=1.2 r=0.36 turn=0
zone yard1 treasure size=0.8 r=0.38 turn=1/8
zone gate1 treasure size=0.8 r=0.2 turn=0
zone field1 treasure rich r=0.22 turn=1/8
zone p2 start size=1.2 r=0.36 turn=1/4
zone yard2 treasure size=0.8 r=0.38 turn=3/8
zone gate2 treasure size=0.8 r=0.2 turn=1/4
zone field2 treasure rich r=0.22 turn=3/8
zone p3 start size=1.2 r=0.36 turn=1/2
zone yard3 treasure size=0.8 r=0.38 turn=5/8
zone gate3 treasure size=0.8 r=0.2 turn=1/2
zone field3 treasure rich r=0.22 turn=5/8
zone p4 start size=1.2 r=0.36 turn=3/4
zone yard4 treasure size=0.8 r=0.38 turn=7/8
zone gate4 treasure size=0.8 r=0.2 turn=3/4
zone field4 treasure rich r=0.22 turn=7/8
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 yard1 weak
link p1 gate1 strong
link gate1 field1 normal
link p2 yard2 weak
link p2 gate2 strong
link gate2 field2 normal
link p3 yard3 weak
link p3 gate3 strong
link gate3 field3 normal
link p4 yard4 weak
link p4 gate4 strong
link gate4 field4 normal
link field1 gate2 normal
link field2 gate3 normal
link field3 gate4 normal
link field4 gate1 normal
link field1 middle strong
link field2 middle strong
link field3 middle strong
link field4 middle strong
)tpl"
     R"tpl(
name Fortress
players 5
sizes medium

zone p1 start size=1.2 r=0.36 turn=0
zone yard1 treasure size=0.8 r=0.38 turn=1/10
zone gate1 treasure size=0.8 r=0.2 turn=0
zone field1 treasure r=0.22 turn=1/10
zone p2 start size=1.2 r=0.36 turn=1/5
zone yard2 treasure size=0.8 r=0.38 turn=3/10
zone gate2 treasure size=0.8 r=0.2 turn=1/5
zone field2 treasure r=0.22 turn=3/10
zone p3 start size=1.2 r=0.36 turn=2/5
zone yard3 treasure size=0.8 r=0.38 turn=1/2
zone gate3 treasure size=0.8 r=0.2 turn=2/5
zone field3 treasure r=0.22 turn=1/2
zone p4 start size=1.2 r=0.36 turn=3/5
zone yard4 treasure size=0.8 r=0.38 turn=7/10
zone gate4 treasure size=0.8 r=0.2 turn=3/5
zone field4 treasure r=0.22 turn=7/10
zone p5 start size=1.2 r=0.36 turn=4/5
zone yard5 treasure size=0.8 r=0.38 turn=9/10
zone gate5 treasure size=0.8 r=0.2 turn=4/5
zone field5 treasure r=0.22 turn=9/10
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 yard1 weak
link p1 gate1 strong
link gate1 field1 normal
link p2 yard2 weak
link p2 gate2 strong
link gate2 field2 normal
link p3 yard3 weak
link p3 gate3 strong
link gate3 field3 normal
link p4 yard4 weak
link p4 gate4 strong
link gate4 field4 normal
link p5 yard5 weak
link p5 gate5 strong
link gate5 field5 normal
link field1 gate2 normal
link field2 gate3 normal
link field3 gate4 normal
link field4 gate5 normal
link field5 gate1 normal
link field1 middle strong
link field2 middle strong
link field3 middle strong
link field4 middle strong
link field5 middle strong
)tpl"
     R"tpl(
name Fortress
players 5
sizes large-huge

zone p1 start size=1.2 r=0.36 turn=0
zone yard1 treasure size=0.8 r=0.38 turn=1/10
zone gate1 treasure size=0.8 r=0.2 turn=0
zone field1 treasure rich r=0.22 turn=1/10
zone p2 start size=1.2 r=0.36 turn=1/5
zone yard2 treasure size=0.8 r=0.38 turn=3/10
zone gate2 treasure size=0.8 r=0.2 turn=1/5
zone field2 treasure rich r=0.22 turn=3/10
zone p3 start size=1.2 r=0.36 turn=2/5
zone yard3 treasure size=0.8 r=0.38 turn=1/2
zone gate3 treasure size=0.8 r=0.2 turn=2/5
zone field3 treasure rich r=0.22 turn=1/2
zone p4 start size=1.2 r=0.36 turn=3/5
zone yard4 treasure size=0.8 r=0.38 turn=7/10
zone gate4 treasure size=0.8 r=0.2 turn=3/5
zone field4 treasure rich r=0.22 turn=7/10
zone p5 start size=1.2 r=0.36 turn=4/5
zone yard5 treasure size=0.8 r=0.38 turn=9/10
zone gate5 treasure size=0.8 r=0.2 turn=4/5
zone field5 treasure rich r=0.22 turn=9/10
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 yard1 weak
link p1 gate1 strong
link gate1 field1 normal
link p2 yard2 weak
link p2 gate2 strong
link gate2 field2 normal
link p3 yard3 weak
link p3 gate3 strong
link gate3 field3 normal
link p4 yard4 weak
link p4 gate4 strong
link gate4 field4 normal
link p5 yard5 weak
link p5 gate5 strong
link gate5 field5 normal
link field1 gate2 normal
link field2 gate3 normal
link field3 gate4 normal
link field4 gate5 normal
link field5 gate1 normal
link field1 middle strong
link field2 middle strong
link field3 middle strong
link field4 middle strong
link field5 middle strong
)tpl"
     R"tpl(
name Fortress
players 6
sizes medium

zone p1 start size=1.2 r=0.36 turn=0
zone yard1 treasure size=0.8 r=0.38 turn=1/12
zone gate1 treasure size=0.8 r=0.2 turn=0
zone field1 treasure r=0.22 turn=1/12
zone p2 start size=1.2 r=0.36 turn=1/6
zone yard2 treasure size=0.8 r=0.38 turn=1/4
zone gate2 treasure size=0.8 r=0.2 turn=1/6
zone field2 treasure r=0.22 turn=1/4
zone p3 start size=1.2 r=0.36 turn=1/3
zone yard3 treasure size=0.8 r=0.38 turn=5/12
zone gate3 treasure size=0.8 r=0.2 turn=1/3
zone field3 treasure r=0.22 turn=5/12
zone p4 start size=1.2 r=0.36 turn=1/2
zone yard4 treasure size=0.8 r=0.38 turn=7/12
zone gate4 treasure size=0.8 r=0.2 turn=1/2
zone field4 treasure r=0.22 turn=7/12
zone p5 start size=1.2 r=0.36 turn=2/3
zone yard5 treasure size=0.8 r=0.38 turn=3/4
zone gate5 treasure size=0.8 r=0.2 turn=2/3
zone field5 treasure r=0.22 turn=3/4
zone p6 start size=1.2 r=0.36 turn=5/6
zone yard6 treasure size=0.8 r=0.38 turn=11/12
zone gate6 treasure size=0.8 r=0.2 turn=5/6
zone field6 treasure r=0.22 turn=11/12
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 yard1 weak
link p1 gate1 strong
link gate1 field1 normal
link p2 yard2 weak
link p2 gate2 strong
link gate2 field2 normal
link p3 yard3 weak
link p3 gate3 strong
link gate3 field3 normal
link p4 yard4 weak
link p4 gate4 strong
link gate4 field4 normal
link p5 yard5 weak
link p5 gate5 strong
link gate5 field5 normal
link p6 yard6 weak
link p6 gate6 strong
link gate6 field6 normal
link field1 gate2 normal
link field2 gate3 normal
link field3 gate4 normal
link field4 gate5 normal
link field5 gate6 normal
link field6 gate1 normal
link field1 middle strong
link field2 middle strong
link field3 middle strong
link field4 middle strong
link field5 middle strong
link field6 middle strong
)tpl"
     R"tpl(
name Fortress
players 6
sizes large-huge

zone p1 start size=1.2 r=0.36 turn=0
zone yard1 treasure size=0.8 r=0.38 turn=1/12
zone gate1 treasure size=0.8 r=0.2 turn=0
zone field1 treasure rich r=0.22 turn=1/12
zone p2 start size=1.2 r=0.36 turn=1/6
zone yard2 treasure size=0.8 r=0.38 turn=1/4
zone gate2 treasure size=0.8 r=0.2 turn=1/6
zone field2 treasure rich r=0.22 turn=1/4
zone p3 start size=1.2 r=0.36 turn=1/3
zone yard3 treasure size=0.8 r=0.38 turn=5/12
zone gate3 treasure size=0.8 r=0.2 turn=1/3
zone field3 treasure rich r=0.22 turn=5/12
zone p4 start size=1.2 r=0.36 turn=1/2
zone yard4 treasure size=0.8 r=0.38 turn=7/12
zone gate4 treasure size=0.8 r=0.2 turn=1/2
zone field4 treasure rich r=0.22 turn=7/12
zone p5 start size=1.2 r=0.36 turn=2/3
zone yard5 treasure size=0.8 r=0.38 turn=3/4
zone gate5 treasure size=0.8 r=0.2 turn=2/3
zone field5 treasure rich r=0.22 turn=3/4
zone p6 start size=1.2 r=0.36 turn=5/6
zone yard6 treasure size=0.8 r=0.38 turn=11/12
zone gate6 treasure size=0.8 r=0.2 turn=5/6
zone field6 treasure rich r=0.22 turn=11/12
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 yard1 weak
link p1 gate1 strong
link gate1 field1 normal
link p2 yard2 weak
link p2 gate2 strong
link gate2 field2 normal
link p3 yard3 weak
link p3 gate3 strong
link gate3 field3 normal
link p4 yard4 weak
link p4 gate4 strong
link gate4 field4 normal
link p5 yard5 weak
link p5 gate5 strong
link gate5 field5 normal
link p6 yard6 weak
link p6 gate6 strong
link gate6 field6 normal
link field1 gate2 normal
link field2 gate3 normal
link field3 gate4 normal
link field4 gate5 normal
link field5 gate6 normal
link field6 gate1 normal
link field1 middle strong
link field2 middle strong
link field3 middle strong
link field4 middle strong
link field5 middle strong
link field6 middle strong
)tpl"},
    {"frontier.txt",
     R"tpl(# Frontier: two lands of three zones, a line of strongly guarded border zones between them.
)tpl"
     R"tpl(
name Frontier
players 2
sizes small-medium

zone p1 start size=1.2 x=-0.34 y=0
zone home1a treasure x=-0.3 y=-0.3
zone home1b treasure x=-0.3 y=0.3
zone p2 start size=1.2 x=0.34 y=0
zone home2a treasure x=0.3 y=-0.3
zone home2b treasure x=0.3 y=0.3
zone border1 treasure size=0.8 x=0 y=-0.3
zone border2 treasure rich town size=0.8 x=0 y=0
zone border3 treasure size=0.8 x=0 y=0.3
link p1 home1a weak
link p1 home1b weak
link p2 home2a weak
link p2 home2b weak
link home1a border1 strong
link home2a border1 strong
link p1 border2 strong
link p2 border2 strong
link home1b border3 strong
link home2b border3 strong
)tpl"
     R"tpl(
name Frontier
players 2
sizes large-huge

zone p1 start size=1.2 x=-0.34 y=0
zone home1a treasure x=-0.3 y=-0.3
zone home1b treasure x=-0.3 y=0.3
zone p2 start size=1.2 x=0.34 y=0
zone home2a treasure x=0.3 y=-0.3
zone home2b treasure x=0.3 y=0.3
zone border1 treasure size=0.8 x=0 y=-0.3
zone border2 treasure rich size=0.8 x=0 y=-0.1
zone border3 treasure town size=0.8 x=0 y=0.1
zone border4 treasure rich size=0.8 x=0 y=0.3
link p1 home1a weak
link p1 home1b weak
link p2 home2a weak
link p2 home2b weak
link home1a border1 strong
link home2a border1 strong
link p1 border2 strong
link p2 border2 strong
link p1 border3 strong
link p2 border3 strong
link home1b border4 strong
link home2b border4 strong
)tpl"},
    {"gauntlet.txt",
     R"tpl(# Gauntlet: each start's way to the prize in the middle, zone after zone guarded more each time.
)tpl"
     R"tpl(
name Gauntlet
players 2
sizes small-medium

zone prize treasure rich town size=0.8 r=0 turn=0
zone p1 start r=0.38 turn=0
zone trial1a treasure rich r=0.26 turn=0
zone p2 start r=0.38 turn=1/2
zone trial2a treasure rich r=0.26 turn=1/2
link p1 trial1a normal
link trial1a prize deadly
link p2 trial2a normal
link trial2a prize deadly
link trial1a trial2a strong
)tpl"
     R"tpl(
name Gauntlet
players 2
sizes large-huge

zone prize treasure rich town size=0.8 r=0 turn=0
zone p1 start r=0.38 turn=0
zone trial1a treasure r=0.29 turn=0
zone trial1b treasure rich r=0.2 turn=1/8
zone p2 start r=0.38 turn=1/2
zone trial2a treasure r=0.29 turn=1/2
zone trial2b treasure rich r=0.2 turn=5/8
link p1 trial1a normal
link trial1a trial1b strong
link trial1b prize deadly
link p2 trial2a normal
link trial2a trial2b strong
link trial2b prize deadly
link trial1a trial2a strong
)tpl"
     R"tpl(
name Gauntlet
players 3
sizes small-medium

zone prize treasure rich town size=0.8 r=0 turn=0
zone p1 start r=0.38 turn=0
zone trial1a treasure rich r=0.26 turn=0
zone p2 start r=0.38 turn=1/3
zone trial2a treasure rich r=0.26 turn=1/3
zone p3 start r=0.38 turn=2/3
zone trial3a treasure rich r=0.26 turn=2/3
link p1 trial1a normal
link trial1a prize deadly
link p2 trial2a normal
link trial2a prize deadly
link p3 trial3a normal
link trial3a prize deadly
link trial1a trial2a strong
link trial2a trial3a strong
link trial3a trial1a strong
)tpl"
     R"tpl(
name Gauntlet
players 3
sizes large-huge

zone prize treasure rich town size=0.8 r=0 turn=0
zone p1 start r=0.38 turn=0
zone trial1a treasure r=0.29 turn=0
zone trial1b treasure rich r=0.2 turn=1/12
zone p2 start r=0.38 turn=1/3
zone trial2a treasure r=0.29 turn=1/3
zone trial2b treasure rich r=0.2 turn=5/12
zone p3 start r=0.38 turn=2/3
zone trial3a treasure r=0.29 turn=2/3
zone trial3b treasure rich r=0.2 turn=3/4
link p1 trial1a normal
link trial1a trial1b strong
link trial1b prize deadly
link p2 trial2a normal
link trial2a trial2b strong
link trial2b prize deadly
link p3 trial3a normal
link trial3a trial3b strong
link trial3b prize deadly
link trial1a trial2a strong
link trial2a trial3a strong
link trial3a trial1a strong
)tpl"
     R"tpl(
name Gauntlet
players 4
sizes small-medium

zone prize treasure rich town size=0.8 r=0 turn=0
zone p1 start r=0.38 turn=0
zone trial1a treasure rich r=0.26 turn=0
zone p2 start r=0.38 turn=1/4
zone trial2a treasure rich r=0.26 turn=1/4
zone p3 start r=0.38 turn=1/2
zone trial3a treasure rich r=0.26 turn=1/2
zone p4 start r=0.38 turn=3/4
zone trial4a treasure rich r=0.26 turn=3/4
link p1 trial1a normal
link trial1a prize deadly
link p2 trial2a normal
link trial2a prize deadly
link p3 trial3a normal
link trial3a prize deadly
link p4 trial4a normal
link trial4a prize deadly
link trial1a trial2a strong
link trial2a trial3a strong
link trial3a trial4a strong
link trial4a trial1a strong
)tpl"
     R"tpl(
name Gauntlet
players 4
sizes large-huge

zone prize treasure rich town size=0.8 r=0 turn=0
zone p1 start r=0.38 turn=0
zone trial1a treasure r=0.29 turn=0
zone trial1b treasure rich r=0.2 turn=1/16
zone p2 start r=0.38 turn=1/4
zone trial2a treasure r=0.29 turn=1/4
zone trial2b treasure rich r=0.2 turn=5/16
zone p3 start r=0.38 turn=1/2
zone trial3a treasure r=0.29 turn=1/2
zone trial3b treasure rich r=0.2 turn=9/16
zone p4 start r=0.38 turn=3/4
zone trial4a treasure r=0.29 turn=3/4
zone trial4b treasure rich r=0.2 turn=13/16
link p1 trial1a normal
link trial1a trial1b strong
link trial1b prize deadly
link p2 trial2a normal
link trial2a trial2b strong
link trial2b prize deadly
link p3 trial3a normal
link trial3a trial3b strong
link trial3b prize deadly
link p4 trial4a normal
link trial4a trial4b strong
link trial4b prize deadly
link trial1a trial2a strong
link trial2a trial3a strong
link trial3a trial4a strong
link trial4a trial1a strong
)tpl"
     R"tpl(
name Gauntlet
players 5
sizes small-medium

zone prize treasure rich town size=0.8 r=0 turn=0
zone p1 start r=0.38 turn=0
zone trial1a treasure rich r=0.26 turn=0
zone p2 start r=0.38 turn=1/5
zone trial2a treasure rich r=0.26 turn=1/5
zone p3 start r=0.38 turn=2/5
zone trial3a treasure rich r=0.26 turn=2/5
zone p4 start r=0.38 turn=3/5
zone trial4a treasure rich r=0.26 turn=3/5
zone p5 start r=0.38 turn=4/5
zone trial5a treasure rich r=0.26 turn=4/5
link p1 trial1a normal
link trial1a prize deadly
link p2 trial2a normal
link trial2a prize deadly
link p3 trial3a normal
link trial3a prize deadly
link p4 trial4a normal
link trial4a prize deadly
link p5 trial5a normal
link trial5a prize deadly
link trial1a trial2a strong
link trial2a trial3a strong
link trial3a trial4a strong
link trial4a trial5a strong
link trial5a trial1a strong
)tpl"
     R"tpl(
name Gauntlet
players 5
sizes large-huge

zone prize treasure rich town size=0.8 r=0 turn=0
zone p1 start r=0.38 turn=0
zone trial1a treasure r=0.29 turn=0
zone trial1b treasure rich r=0.2 turn=1/20
zone p2 start r=0.38 turn=1/5
zone trial2a treasure r=0.29 turn=1/5
zone trial2b treasure rich r=0.2 turn=1/4
zone p3 start r=0.38 turn=2/5
zone trial3a treasure r=0.29 turn=2/5
zone trial3b treasure rich r=0.2 turn=9/20
zone p4 start r=0.38 turn=3/5
zone trial4a treasure r=0.29 turn=3/5
zone trial4b treasure rich r=0.2 turn=13/20
zone p5 start r=0.38 turn=4/5
zone trial5a treasure r=0.29 turn=4/5
zone trial5b treasure rich r=0.2 turn=17/20
link p1 trial1a normal
link trial1a trial1b strong
link trial1b prize deadly
link p2 trial2a normal
link trial2a trial2b strong
link trial2b prize deadly
link p3 trial3a normal
link trial3a trial3b strong
link trial3b prize deadly
link p4 trial4a normal
link trial4a trial4b strong
link trial4b prize deadly
link p5 trial5a normal
link trial5a trial5b strong
link trial5b prize deadly
link trial1a trial2a strong
link trial2a trial3a strong
link trial3a trial4a strong
link trial4a trial5a strong
link trial5a trial1a strong
)tpl"
     R"tpl(
name Gauntlet
players 6
sizes small-medium

zone prize treasure rich town size=0.8 r=0 turn=0
zone p1 start r=0.38 turn=0
zone trial1a treasure rich r=0.26 turn=0
zone p2 start r=0.38 turn=1/6
zone trial2a treasure rich r=0.26 turn=1/6
zone p3 start r=0.38 turn=1/3
zone trial3a treasure rich r=0.26 turn=1/3
zone p4 start r=0.38 turn=1/2
zone trial4a treasure rich r=0.26 turn=1/2
zone p5 start r=0.38 turn=2/3
zone trial5a treasure rich r=0.26 turn=2/3
zone p6 start r=0.38 turn=5/6
zone trial6a treasure rich r=0.26 turn=5/6
link p1 trial1a normal
link trial1a prize deadly
link p2 trial2a normal
link trial2a prize deadly
link p3 trial3a normal
link trial3a prize deadly
link p4 trial4a normal
link trial4a prize deadly
link p5 trial5a normal
link trial5a prize deadly
link p6 trial6a normal
link trial6a prize deadly
link trial1a trial2a strong
link trial2a trial3a strong
link trial3a trial4a strong
link trial4a trial5a strong
link trial5a trial6a strong
link trial6a trial1a strong
)tpl"
     R"tpl(
name Gauntlet
players 6
sizes large-huge

zone prize treasure rich town size=0.8 r=0 turn=0
zone p1 start r=0.38 turn=0
zone trial1a treasure r=0.29 turn=0
zone trial1b treasure rich r=0.2 turn=1/24
zone p2 start r=0.38 turn=1/6
zone trial2a treasure r=0.29 turn=1/6
zone trial2b treasure rich r=0.2 turn=5/24
zone p3 start r=0.38 turn=1/3
zone trial3a treasure r=0.29 turn=1/3
zone trial3b treasure rich r=0.2 turn=3/8
zone p4 start r=0.38 turn=1/2
zone trial4a treasure r=0.29 turn=1/2
zone trial4b treasure rich r=0.2 turn=13/24
zone p5 start r=0.38 turn=2/3
zone trial5a treasure r=0.29 turn=2/3
zone trial5b treasure rich r=0.2 turn=17/24
zone p6 start r=0.38 turn=5/6
zone trial6a treasure r=0.29 turn=5/6
zone trial6b treasure rich r=0.2 turn=7/8
link p1 trial1a normal
link trial1a trial1b strong
link trial1b prize deadly
link p2 trial2a normal
link trial2a trial2b strong
link trial2b prize deadly
link p3 trial3a normal
link trial3a trial3b strong
link trial3b prize deadly
link p4 trial4a normal
link trial4a trial4b strong
link trial4b prize deadly
link p5 trial5a normal
link trial5a trial5b strong
link trial5b prize deadly
link p6 trial6a normal
link trial6a trial6b strong
link trial6b prize deadly
link trial1a trial2a strong
link trial2a trial3a strong
link trial3a trial4a strong
link trial4a trial5a strong
link trial5a trial6a strong
link trial6a trial1a strong
)tpl"},
    {"horseshoe.txt",
     R"tpl(# Horseshoe: two players at a horseshoe's ends, the riches round its bend; a deadly short cut
# across its opening.
)tpl"
     R"tpl(
name Horseshoe
players 2
sizes small-medium

zone p1 start r=0.33 turn=1/8
zone bend1 treasure r=0.33 turn=5/16
zone bend2 treasure rich town r=0.33 turn=1/2
zone bend3 treasure r=0.33 turn=11/16
zone p2 start r=0.33 turn=7/8
zone gap treasure size=0.7 r=0.1 turn=0
link p1 bend1 normal
link bend1 bend2 strong
link bend2 bend3 strong
link bend3 p2 normal
link p1 gap deadly
link p2 gap deadly
)tpl"
     R"tpl(
name Horseshoe
players 2
sizes large-huge

zone p1 start r=0.33 turn=1/8
zone bend1 treasure r=0.33 turn=1/4
zone bend2 treasure r=0.33 turn=3/8
zone bend3 treasure rich town r=0.33 turn=1/2
zone bend4 treasure r=0.33 turn=5/8
zone bend5 treasure r=0.33 turn=3/4
zone p2 start r=0.33 turn=7/8
zone gap treasure size=0.7 r=0.1 turn=0
link p1 bend1 normal
link bend1 bend2 normal
link bend2 bend3 strong
link bend3 bend4 strong
link bend4 bend5 normal
link bend5 p2 normal
link p1 gap deadly
link p2 gap deadly
)tpl"},
    {"hourglass.txt",
     R"tpl(# Hourglass: two players; each half's zones meet at its gate, the gates at one narrow rich
# middle with a town, deadly guarded.
)tpl"
     R"tpl(
name Hourglass
players 2
sizes small-medium

zone neck treasure rich town size=0.8 x=0 y=0
zone p1 start size=1.3 x=-0.33 y=0
zone bulb1a treasure x=-0.2 y=-0.28
zone bulb1b treasure x=-0.2 y=0.28
zone gate1 treasure size=0.8 x=-0.14 y=0
zone p2 start size=1.3 x=0.33 y=0
zone bulb2a treasure x=0.2 y=-0.28
zone bulb2b treasure x=0.2 y=0.28
zone gate2 treasure size=0.8 x=0.14 y=0
link p1 bulb1a weak
link p1 bulb1b weak
link bulb1a gate1 normal
link bulb1b gate1 normal
link gate1 neck deadly
link p2 bulb2a weak
link p2 bulb2b weak
link bulb2a gate2 normal
link bulb2b gate2 normal
link gate2 neck deadly
)tpl"
     R"tpl(
name Hourglass
players 2
sizes large-huge

zone neck treasure rich town size=0.8 x=0 y=0
zone p1 start size=1.3 x=-0.33 y=0
zone bulb1a treasure x=-0.2 y=-0.28
zone bulb1b treasure x=-0.2 y=0.28
zone gate1 treasure size=0.8 x=-0.14 y=0
zone bulb1c treasure rich size=0.8 x=-0.36 y=-0.38
zone bulb1d treasure rich size=0.8 x=-0.36 y=0.38
zone p2 start size=1.3 x=0.33 y=0
zone bulb2a treasure x=0.2 y=-0.28
zone bulb2b treasure x=0.2 y=0.28
zone gate2 treasure size=0.8 x=0.14 y=0
zone bulb2c treasure rich size=0.8 x=0.36 y=-0.38
zone bulb2d treasure rich size=0.8 x=0.36 y=0.38
link p1 bulb1a weak
link p1 bulb1b weak
link bulb1a gate1 normal
link bulb1b gate1 normal
link gate1 neck deadly
link bulb1a bulb1c strong
link bulb1b bulb1d strong
link p2 bulb2a weak
link p2 bulb2b weak
link bulb2a gate2 normal
link bulb2b gate2 normal
link gate2 neck deadly
link bulb2a bulb2c strong
link bulb2b bulb2d strong
)tpl"},
    {"hub.txt",
     R"tpl(# Hub: the players' starts round a rich, deadly guarded middle with a town; each start with its
# own treasure zone on the way in; from medium maps up, treasure zones between neighbours.
# One variant for each number of players and the map sizes that share a layout.
)tpl"
     R"tpl(
name Hub
players 2
sizes small

zone centre1 treasure rich town size=1.4 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure size=0.7 r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/2
zone own2 treasure size=0.7 r=0.2 turn=1/2
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
)tpl"
     R"tpl(
name Hub
players 2
sizes medium

zone centre1 treasure rich town size=1.4 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/2
zone own2 treasure r=0.2 turn=1/2
zone between1 treasure r=0.38 turn=1/4
zone between2 treasure r=0.38 turn=3/4
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p1 between1
link between1 p2
link p2 between2
link between2 p1
)tpl"
     R"tpl(
name Hub
players 2
sizes large-huge

zone centre1 treasure rich town size=1.4 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/2
zone own2 treasure r=0.2 turn=1/2
zone between1 treasure rich r=0.38 turn=1/4
zone between2 treasure rich r=0.38 turn=3/4
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p1 between1
link between1 p2
link p2 between2
link between2 p1
)tpl"
     R"tpl(
name Hub
players 3
sizes small

zone centre1 treasure rich town size=1.5 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure size=0.7 r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/3
zone own2 treasure size=0.7 r=0.2 turn=1/3
zone p3 start size=1.2 r=0.37 turn=2/3
zone own3 treasure size=0.7 r=0.2 turn=2/3
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p3 own3
link own3 centre1 deadly
)tpl"
     R"tpl(
name Hub
players 3
sizes medium

zone centre1 treasure rich town size=1.5 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/3
zone own2 treasure r=0.2 turn=1/3
zone p3 start size=1.2 r=0.37 turn=2/3
zone own3 treasure r=0.2 turn=2/3
zone between1 treasure r=0.38 turn=1/6
zone between2 treasure r=0.38 turn=1/2
zone between3 treasure r=0.38 turn=5/6
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p3 own3
link own3 centre1 deadly
link p1 between1
link between1 p2
link p2 between2
link between2 p3
link p3 between3
link between3 p1
)tpl"
     R"tpl(
name Hub
players 3
sizes large-huge

zone centre1 treasure rich town size=1.5 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/3
zone own2 treasure r=0.2 turn=1/3
zone p3 start size=1.2 r=0.37 turn=2/3
zone own3 treasure r=0.2 turn=2/3
zone between1 treasure rich r=0.38 turn=1/6
zone between2 treasure rich r=0.38 turn=1/2
zone between3 treasure rich r=0.38 turn=5/6
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p3 own3
link own3 centre1 deadly
link p1 between1
link between1 p2
link p2 between2
link between2 p3
link p3 between3
link between3 p1
)tpl"
     R"tpl(
name Hub
players 4
sizes small

zone centre1 treasure rich town size=1.6 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure size=0.7 r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/4
zone own2 treasure size=0.7 r=0.2 turn=1/4
zone p3 start size=1.2 r=0.37 turn=1/2
zone own3 treasure size=0.7 r=0.2 turn=1/2
zone p4 start size=1.2 r=0.37 turn=3/4
zone own4 treasure size=0.7 r=0.2 turn=3/4
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p3 own3
link own3 centre1 deadly
link p4 own4
link own4 centre1 deadly
)tpl"
     R"tpl(
name Hub
players 4
sizes medium

zone centre1 treasure rich town size=1.6 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/4
zone own2 treasure r=0.2 turn=1/4
zone p3 start size=1.2 r=0.37 turn=1/2
zone own3 treasure r=0.2 turn=1/2
zone p4 start size=1.2 r=0.37 turn=3/4
zone own4 treasure r=0.2 turn=3/4
zone between1 treasure r=0.38 turn=1/8
zone between2 treasure r=0.38 turn=3/8
zone between3 treasure r=0.38 turn=5/8
zone between4 treasure r=0.38 turn=7/8
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p3 own3
link own3 centre1 deadly
link p4 own4
link own4 centre1 deadly
link p1 between1
link between1 p2
link p2 between2
link between2 p3
link p3 between3
link between3 p4
link p4 between4
link between4 p1
)tpl"
     R"tpl(
name Hub
players 4
sizes large-huge

zone centre1 treasure rich town size=1.6 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/4
zone own2 treasure r=0.2 turn=1/4
zone p3 start size=1.2 r=0.37 turn=1/2
zone own3 treasure r=0.2 turn=1/2
zone p4 start size=1.2 r=0.37 turn=3/4
zone own4 treasure r=0.2 turn=3/4
zone between1 treasure rich r=0.38 turn=1/8
zone between2 treasure rich r=0.38 turn=3/8
zone between3 treasure rich r=0.38 turn=5/8
zone between4 treasure rich r=0.38 turn=7/8
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p3 own3
link own3 centre1 deadly
link p4 own4
link own4 centre1 deadly
link p1 between1
link between1 p2
link p2 between2
link between2 p3
link p3 between3
link between3 p4
link p4 between4
link between4 p1
)tpl"
     R"tpl(
name Hub
players 5
sizes small

zone centre1 treasure rich town size=1.7 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure size=0.7 r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/5
zone own2 treasure size=0.7 r=0.2 turn=1/5
zone p3 start size=1.2 r=0.37 turn=2/5
zone own3 treasure size=0.7 r=0.2 turn=2/5
zone p4 start size=1.2 r=0.37 turn=3/5
zone own4 treasure size=0.7 r=0.2 turn=3/5
zone p5 start size=1.2 r=0.37 turn=4/5
zone own5 treasure size=0.7 r=0.2 turn=4/5
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p3 own3
link own3 centre1 deadly
link p4 own4
link own4 centre1 deadly
link p5 own5
link own5 centre1 deadly
)tpl"
     R"tpl(
name Hub
players 5
sizes medium

zone centre1 treasure rich town size=1.7 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/5
zone own2 treasure r=0.2 turn=1/5
zone p3 start size=1.2 r=0.37 turn=2/5
zone own3 treasure r=0.2 turn=2/5
zone p4 start size=1.2 r=0.37 turn=3/5
zone own4 treasure r=0.2 turn=3/5
zone p5 start size=1.2 r=0.37 turn=4/5
zone own5 treasure r=0.2 turn=4/5
zone between1 treasure r=0.38 turn=1/10
zone between2 treasure r=0.38 turn=3/10
zone between3 treasure r=0.38 turn=1/2
zone between4 treasure r=0.38 turn=7/10
zone between5 treasure r=0.38 turn=9/10
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p3 own3
link own3 centre1 deadly
link p4 own4
link own4 centre1 deadly
link p5 own5
link own5 centre1 deadly
link p1 between1
link between1 p2
link p2 between2
link between2 p3
link p3 between3
link between3 p4
link p4 between4
link between4 p5
link p5 between5
link between5 p1
)tpl"
     R"tpl(
name Hub
players 5
sizes large-huge

zone centre1 treasure rich town size=1.7 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/5
zone own2 treasure r=0.2 turn=1/5
zone p3 start size=1.2 r=0.37 turn=2/5
zone own3 treasure r=0.2 turn=2/5
zone p4 start size=1.2 r=0.37 turn=3/5
zone own4 treasure r=0.2 turn=3/5
zone p5 start size=1.2 r=0.37 turn=4/5
zone own5 treasure r=0.2 turn=4/5
zone between1 treasure rich r=0.38 turn=1/10
zone between2 treasure rich r=0.38 turn=3/10
zone between3 treasure rich r=0.38 turn=1/2
zone between4 treasure rich r=0.38 turn=7/10
zone between5 treasure rich r=0.38 turn=9/10
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p3 own3
link own3 centre1 deadly
link p4 own4
link own4 centre1 deadly
link p5 own5
link own5 centre1 deadly
link p1 between1
link between1 p2
link p2 between2
link between2 p3
link p3 between3
link between3 p4
link p4 between4
link between4 p5
link p5 between5
link between5 p1
)tpl"
     R"tpl(
name Hub
players 6
sizes small

zone centre1 treasure rich town size=1.8 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure size=0.7 r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/6
zone own2 treasure size=0.7 r=0.2 turn=1/6
zone p3 start size=1.2 r=0.37 turn=1/3
zone own3 treasure size=0.7 r=0.2 turn=1/3
zone p4 start size=1.2 r=0.37 turn=1/2
zone own4 treasure size=0.7 r=0.2 turn=1/2
zone p5 start size=1.2 r=0.37 turn=2/3
zone own5 treasure size=0.7 r=0.2 turn=2/3
zone p6 start size=1.2 r=0.37 turn=5/6
zone own6 treasure size=0.7 r=0.2 turn=5/6
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p3 own3
link own3 centre1 deadly
link p4 own4
link own4 centre1 deadly
link p5 own5
link own5 centre1 deadly
link p6 own6
link own6 centre1 deadly
)tpl"
     R"tpl(
name Hub
players 6
sizes medium

zone centre1 treasure rich town size=1.8 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/6
zone own2 treasure r=0.2 turn=1/6
zone p3 start size=1.2 r=0.37 turn=1/3
zone own3 treasure r=0.2 turn=1/3
zone p4 start size=1.2 r=0.37 turn=1/2
zone own4 treasure r=0.2 turn=1/2
zone p5 start size=1.2 r=0.37 turn=2/3
zone own5 treasure r=0.2 turn=2/3
zone p6 start size=1.2 r=0.37 turn=5/6
zone own6 treasure r=0.2 turn=5/6
zone between1 treasure r=0.38 turn=1/12
zone between2 treasure r=0.38 turn=1/4
zone between3 treasure r=0.38 turn=5/12
zone between4 treasure r=0.38 turn=7/12
zone between5 treasure r=0.38 turn=3/4
zone between6 treasure r=0.38 turn=11/12
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p3 own3
link own3 centre1 deadly
link p4 own4
link own4 centre1 deadly
link p5 own5
link own5 centre1 deadly
link p6 own6
link own6 centre1 deadly
link p1 between1
link between1 p2
link p2 between2
link between2 p3
link p3 between3
link between3 p4
link p4 between4
link between4 p5
link p5 between5
link between5 p6
link p6 between6
link between6 p1
)tpl"
     R"tpl(
name Hub
players 6
sizes large-huge

zone centre1 treasure rich town size=1.8 x=0 y=0
zone p1 start size=1.2 r=0.37 turn=0
zone own1 treasure r=0.2 turn=0
zone p2 start size=1.2 r=0.37 turn=1/6
zone own2 treasure r=0.2 turn=1/6
zone p3 start size=1.2 r=0.37 turn=1/3
zone own3 treasure r=0.2 turn=1/3
zone p4 start size=1.2 r=0.37 turn=1/2
zone own4 treasure r=0.2 turn=1/2
zone p5 start size=1.2 r=0.37 turn=2/3
zone own5 treasure r=0.2 turn=2/3
zone p6 start size=1.2 r=0.37 turn=5/6
zone own6 treasure r=0.2 turn=5/6
zone between1 treasure rich r=0.38 turn=1/12
zone between2 treasure rich r=0.38 turn=1/4
zone between3 treasure rich r=0.38 turn=5/12
zone between4 treasure rich r=0.38 turn=7/12
zone between5 treasure rich r=0.38 turn=3/4
zone between6 treasure rich r=0.38 turn=11/12
link p1 own1
link own1 centre1 deadly
link p2 own2
link own2 centre1 deadly
link p3 own3
link own3 centre1 deadly
link p4 own4
link own4 centre1 deadly
link p5 own5
link own5 centre1 deadly
link p6 own6
link own6 centre1 deadly
link p1 between1
link between1 p2
link p2 between2
link between2 p3
link p3 between3
link between3 p4
link p4 between4
link between4 p5
link p5 between5
link between5 p6
link p6 between6
link between6 p1
)tpl"},
    {"labyrinth.txt",
     R"tpl(# Labyrinth: two players at the ends of a winding line through a grid of zones; one deadly
# short cut.
)tpl"
     R"tpl(
name Labyrinth
players 2
sizes small-medium

zone p1 start x=-0.33 y=-0.33
zone cell1 treasure x=0 y=-0.33
zone cell2 treasure x=0.33 y=-0.33
zone cell3 treasure x=0.33 y=0
zone cell4 treasure rich town x=0 y=0
zone cell5 treasure x=-0.33 y=0
zone cell6 treasure x=-0.33 y=0.33
zone cell7 treasure x=0 y=0.33
zone p2 start x=0.33 y=0.33
link p1 cell1 normal
link cell1 cell2 normal
link cell2 cell3 normal
link cell3 cell4 strong
link cell4 cell5 strong
link cell5 cell6 strong
link cell6 cell7 normal
link cell7 p2 normal
link cell1 cell4 deadly
)tpl"
     R"tpl(
name Labyrinth
players 2
sizes large-huge

zone p1 start x=-0.33 y=-0.33
zone cell1 treasure x=-0.11 y=-0.33
zone cell2 treasure x=0.11 y=-0.33
zone cell3 treasure x=0.33 y=-0.33
zone cell4 treasure x=0.33 y=-0.11
zone cell5 treasure x=0.11 y=-0.11
zone cell6 treasure x=-0.11 y=-0.11
zone cell7 treasure x=-0.33 y=-0.11
zone cell8 treasure rich town x=-0.33 y=0.11
zone cell9 treasure x=-0.11 y=0.11
zone cell10 treasure x=0.11 y=0.11
zone cell11 treasure x=0.33 y=0.11
zone cell12 treasure x=0.33 y=0.33
zone cell13 treasure x=0.11 y=0.33
zone cell14 treasure x=-0.11 y=0.33
zone p2 start x=-0.33 y=0.33
link p1 cell1 normal
link cell1 cell2 normal
link cell2 cell3 normal
link cell3 cell4 normal
link cell4 cell5 normal
link cell5 cell6 normal
link cell6 cell7 normal
link cell7 cell8 strong
link cell8 cell9 strong
link cell9 cell10 strong
link cell10 cell11 normal
link cell11 cell12 normal
link cell12 cell13 normal
link cell13 cell14 normal
link cell14 p2 normal
link cell1 cell6 deadly
)tpl"},
    {"ladder.txt",
     R"tpl(# Ladder: two players, two parallel lines of zones between them with guarded rungs across.
)tpl"
     R"tpl(
name Ladder
players 2
sizes small-medium

zone p1 start size=1.2 x=-0.38 y=0
zone p2 start size=1.2 x=0.38 y=0
zone n1 treasure x=-0.24 y=-0.2
zone n2 treasure rich x=0 y=-0.2
zone n3 treasure x=0.24 y=-0.2
zone s1 treasure x=-0.24 y=0.2
zone s2 treasure rich x=0 y=0.2
zone s3 treasure x=0.24 y=0.2
link p1 n1 weak
link n1 n2 normal
link n2 n3 normal
link n3 p2 weak
link p1 s1 weak
link s1 s2 normal
link s2 s3 normal
link s3 p2 weak
link n2 s2 strong
)tpl"
     R"tpl(
name Ladder
players 2
sizes large-huge

zone p1 start size=1.2 x=-0.38 y=0
zone p2 start size=1.2 x=0.38 y=0
zone n1 treasure x=-0.24 y=-0.2
zone n2 treasure rich x=-0.08 y=-0.2
zone n3 treasure rich x=0.08 y=-0.2
zone n4 treasure x=0.24 y=-0.2
zone s1 treasure x=-0.24 y=0.2
zone s2 treasure rich x=-0.08 y=0.2
zone s3 treasure rich x=0.08 y=0.2
zone s4 treasure x=0.24 y=0.2
link p1 n1 weak
link n1 n2 normal
link n2 n3 normal
link n3 n4 normal
link n4 p2 weak
link p1 s1 weak
link s1 s2 normal
link s2 s3 normal
link s3 s4 normal
link s4 p2 weak
link n2 s2 strong
link n4 s4 strong
)tpl"},
    {"mirror.txt",
     R"tpl(# Mirror: two players, mirrored halves; two rich middles joined by an open way.
)tpl"
     R"tpl(
name Mirror
players 2
sizes small-medium

zone north treasure rich x=0 y=-0.17
zone south treasure rich town x=0 y=0.17
zone p1 start size=1.2 x=-0.33 y=0
zone front1a treasure x=-0.17 y=-0.2
zone front1b treasure x=-0.17 y=0.2
zone back1 treasure size=0.8 x=-0.34 y=0.3
zone p2 start size=1.2 x=0.33 y=0
zone front2a treasure x=0.17 y=-0.2
zone front2b treasure x=0.17 y=0.2
zone back2 treasure size=0.8 x=0.34 y=-0.3
link north south open
link p1 front1a weak
link p1 front1b weak
link front1a north strong
link front1b south strong
link p1 back1 weak
link p2 front2a weak
link p2 front2b weak
link front2a north strong
link front2b south strong
link p2 back2 weak
)tpl"
     R"tpl(
name Mirror
players 2
sizes large-huge

zone north treasure rich x=0 y=-0.17
zone south treasure rich town x=0 y=0.17
zone p1 start size=1.2 x=-0.33 y=0
zone front1a treasure x=-0.17 y=-0.2
zone front1b treasure x=-0.17 y=0.2
zone back1 treasure size=0.8 x=-0.34 y=0.3
zone far1 treasure rich x=-0.34 y=-0.3
zone p2 start size=1.2 x=0.33 y=0
zone front2a treasure x=0.17 y=-0.2
zone front2b treasure x=0.17 y=0.2
zone back2 treasure size=0.8 x=0.34 y=-0.3
zone far2 treasure rich x=0.34 y=0.3
link north south open
link p1 front1a weak
link p1 front1b weak
link front1a north strong
link front1b south strong
link p1 back1 weak
link front1a far1 strong
link p2 front2a weak
link p2 front2b weak
link front2a north strong
link front2b south strong
link p2 back2 weak
link front2b far2 strong
)tpl"},
    {"pockets.txt",
     R"tpl(# Pockets: each start with two private pockets behind it; shared zones in front, between
# neighbours (and a rich middle for 3 players and more).
)tpl"
     R"tpl(
name Pockets
players 2
sizes small-medium

zone p1 start size=1.1 r=0.3 turn=0
zone pocket1a treasure size=0.7 r=0.38 turn=9/10
zone pocket1b treasure size=0.7 r=0.38 turn=1/10
zone ring1 treasure r=0.15 turn=1/4
zone p2 start size=1.1 r=0.3 turn=1/2
zone pocket2a treasure size=0.7 r=0.38 turn=2/5
zone pocket2b treasure size=0.7 r=0.38 turn=3/5
zone ring2 treasure r=0.15 turn=3/4
link p1 pocket1a weak
link p1 pocket1b weak
link p2 pocket2a weak
link p2 pocket2b weak
link p1 ring1 normal
link p2 ring1 normal
link p2 ring2 normal
link p1 ring2 normal
)tpl"
     R"tpl(
name Pockets
players 2
sizes large-huge

zone p1 start size=1.1 r=0.3 turn=0
zone pocket1a treasure size=0.7 r=0.38 turn=9/10
zone pocket1b treasure size=0.7 r=0.38 turn=1/10
zone ring1 treasure rich r=0.15 turn=1/4
zone p2 start size=1.1 r=0.3 turn=1/2
zone pocket2a treasure size=0.7 r=0.38 turn=2/5
zone pocket2b treasure size=0.7 r=0.38 turn=3/5
zone ring2 treasure rich r=0.15 turn=3/4
link p1 pocket1a weak
link p1 pocket1b weak
link p2 pocket2a weak
link p2 pocket2b weak
link p1 ring1 normal
link p2 ring1 normal
link p2 ring2 normal
link p1 ring2 normal
)tpl"
     R"tpl(
name Pockets
players 3
sizes small-medium

zone p1 start size=1.1 r=0.3 turn=0
zone pocket1a treasure size=0.7 r=0.38 turn=14/15
zone pocket1b treasure size=0.7 r=0.38 turn=1/15
zone ring1 treasure r=0.15 turn=1/6
zone p2 start size=1.1 r=0.3 turn=1/3
zone pocket2a treasure size=0.7 r=0.38 turn=4/15
zone pocket2b treasure size=0.7 r=0.38 turn=2/5
zone ring2 treasure r=0.15 turn=1/2
zone p3 start size=1.1 r=0.3 turn=2/3
zone pocket3a treasure size=0.7 r=0.38 turn=3/5
zone pocket3b treasure size=0.7 r=0.38 turn=11/15
zone ring3 treasure r=0.15 turn=5/6
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 pocket1a weak
link p1 pocket1b weak
link p2 pocket2a weak
link p2 pocket2b weak
link p3 pocket3a weak
link p3 pocket3b weak
link p1 ring1 normal
link p2 ring1 normal
link p2 ring2 normal
link p3 ring2 normal
link p3 ring3 normal
link p1 ring3 normal
link ring1 middle strong
link ring2 middle strong
link ring3 middle strong
)tpl"
     R"tpl(
name Pockets
players 3
sizes large-huge

zone p1 start size=1.1 r=0.3 turn=0
zone pocket1a treasure size=0.7 r=0.38 turn=14/15
zone pocket1b treasure size=0.7 r=0.38 turn=1/15
zone ring1 treasure rich r=0.15 turn=1/6
zone p2 start size=1.1 r=0.3 turn=1/3
zone pocket2a treasure size=0.7 r=0.38 turn=4/15
zone pocket2b treasure size=0.7 r=0.38 turn=2/5
zone ring2 treasure rich r=0.15 turn=1/2
zone p3 start size=1.1 r=0.3 turn=2/3
zone pocket3a treasure size=0.7 r=0.38 turn=3/5
zone pocket3b treasure size=0.7 r=0.38 turn=11/15
zone ring3 treasure rich r=0.15 turn=5/6
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 pocket1a weak
link p1 pocket1b weak
link p2 pocket2a weak
link p2 pocket2b weak
link p3 pocket3a weak
link p3 pocket3b weak
link p1 ring1 normal
link p2 ring1 normal
link p2 ring2 normal
link p3 ring2 normal
link p3 ring3 normal
link p1 ring3 normal
link ring1 middle strong
link ring2 middle strong
link ring3 middle strong
)tpl"
     R"tpl(
name Pockets
players 4
sizes medium

zone p1 start size=1.1 r=0.3 turn=0
zone pocket1a treasure size=0.7 r=0.38 turn=19/20
zone pocket1b treasure size=0.7 r=0.38 turn=1/20
zone ring1 treasure r=0.17 turn=1/8
zone p2 start size=1.1 r=0.3 turn=1/4
zone pocket2a treasure size=0.7 r=0.38 turn=1/5
zone pocket2b treasure size=0.7 r=0.38 turn=3/10
zone ring2 treasure r=0.17 turn=3/8
zone p3 start size=1.1 r=0.3 turn=1/2
zone pocket3a treasure size=0.7 r=0.38 turn=9/20
zone pocket3b treasure size=0.7 r=0.38 turn=11/20
zone ring3 treasure r=0.17 turn=5/8
zone p4 start size=1.1 r=0.3 turn=3/4
zone pocket4a treasure size=0.7 r=0.38 turn=7/10
zone pocket4b treasure size=0.7 r=0.38 turn=4/5
zone ring4 treasure r=0.17 turn=7/8
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 pocket1a weak
link p1 pocket1b weak
link p2 pocket2a weak
link p2 pocket2b weak
link p3 pocket3a weak
link p3 pocket3b weak
link p4 pocket4a weak
link p4 pocket4b weak
link p1 ring1 normal
link p2 ring1 normal
link p2 ring2 normal
link p3 ring2 normal
link p3 ring3 normal
link p4 ring3 normal
link p4 ring4 normal
link p1 ring4 normal
link ring1 middle strong
link ring2 middle strong
link ring3 middle strong
link ring4 middle strong
)tpl"
     R"tpl(
name Pockets
players 4
sizes large-huge

zone p1 start size=1.1 r=0.3 turn=0
zone pocket1a treasure size=0.7 r=0.38 turn=19/20
zone pocket1b treasure size=0.7 r=0.38 turn=1/20
zone ring1 treasure rich r=0.17 turn=1/8
zone p2 start size=1.1 r=0.3 turn=1/4
zone pocket2a treasure size=0.7 r=0.38 turn=1/5
zone pocket2b treasure size=0.7 r=0.38 turn=3/10
zone ring2 treasure rich r=0.17 turn=3/8
zone p3 start size=1.1 r=0.3 turn=1/2
zone pocket3a treasure size=0.7 r=0.38 turn=9/20
zone pocket3b treasure size=0.7 r=0.38 turn=11/20
zone ring3 treasure rich r=0.17 turn=5/8
zone p4 start size=1.1 r=0.3 turn=3/4
zone pocket4a treasure size=0.7 r=0.38 turn=7/10
zone pocket4b treasure size=0.7 r=0.38 turn=4/5
zone ring4 treasure rich r=0.17 turn=7/8
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 pocket1a weak
link p1 pocket1b weak
link p2 pocket2a weak
link p2 pocket2b weak
link p3 pocket3a weak
link p3 pocket3b weak
link p4 pocket4a weak
link p4 pocket4b weak
link p1 ring1 normal
link p2 ring1 normal
link p2 ring2 normal
link p3 ring2 normal
link p3 ring3 normal
link p4 ring3 normal
link p4 ring4 normal
link p1 ring4 normal
link ring1 middle strong
link ring2 middle strong
link ring3 middle strong
link ring4 middle strong
)tpl"
     R"tpl(
name Pockets
players 5
sizes medium

zone p1 start size=1.1 r=0.3 turn=0
zone pocket1a treasure size=0.7 r=0.38 turn=24/25
zone pocket1b treasure size=0.7 r=0.38 turn=1/25
zone ring1 treasure r=0.17 turn=1/10
zone p2 start size=1.1 r=0.3 turn=1/5
zone pocket2a treasure size=0.7 r=0.38 turn=4/25
zone pocket2b treasure size=0.7 r=0.38 turn=6/25
zone ring2 treasure r=0.17 turn=3/10
zone p3 start size=1.1 r=0.3 turn=2/5
zone pocket3a treasure size=0.7 r=0.38 turn=9/25
zone pocket3b treasure size=0.7 r=0.38 turn=11/25
zone ring3 treasure r=0.17 turn=1/2
zone p4 start size=1.1 r=0.3 turn=3/5
zone pocket4a treasure size=0.7 r=0.38 turn=14/25
zone pocket4b treasure size=0.7 r=0.38 turn=16/25
zone ring4 treasure r=0.17 turn=7/10
zone p5 start size=1.1 r=0.3 turn=4/5
zone pocket5a treasure size=0.7 r=0.38 turn=19/25
zone pocket5b treasure size=0.7 r=0.38 turn=21/25
zone ring5 treasure r=0.17 turn=9/10
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 pocket1a weak
link p1 pocket1b weak
link p2 pocket2a weak
link p2 pocket2b weak
link p3 pocket3a weak
link p3 pocket3b weak
link p4 pocket4a weak
link p4 pocket4b weak
link p5 pocket5a weak
link p5 pocket5b weak
link p1 ring1 normal
link p2 ring1 normal
link p2 ring2 normal
link p3 ring2 normal
link p3 ring3 normal
link p4 ring3 normal
link p4 ring4 normal
link p5 ring4 normal
link p5 ring5 normal
link p1 ring5 normal
link ring1 middle strong
link ring2 middle strong
link ring3 middle strong
link ring4 middle strong
link ring5 middle strong
)tpl"
     R"tpl(
name Pockets
players 5
sizes large-huge

zone p1 start size=1.1 r=0.3 turn=0
zone pocket1a treasure size=0.7 r=0.38 turn=24/25
zone pocket1b treasure size=0.7 r=0.38 turn=1/25
zone ring1 treasure rich r=0.17 turn=1/10
zone p2 start size=1.1 r=0.3 turn=1/5
zone pocket2a treasure size=0.7 r=0.38 turn=4/25
zone pocket2b treasure size=0.7 r=0.38 turn=6/25
zone ring2 treasure rich r=0.17 turn=3/10
zone p3 start size=1.1 r=0.3 turn=2/5
zone pocket3a treasure size=0.7 r=0.38 turn=9/25
zone pocket3b treasure size=0.7 r=0.38 turn=11/25
zone ring3 treasure rich r=0.17 turn=1/2
zone p4 start size=1.1 r=0.3 turn=3/5
zone pocket4a treasure size=0.7 r=0.38 turn=14/25
zone pocket4b treasure size=0.7 r=0.38 turn=16/25
zone ring4 treasure rich r=0.17 turn=7/10
zone p5 start size=1.1 r=0.3 turn=4/5
zone pocket5a treasure size=0.7 r=0.38 turn=19/25
zone pocket5b treasure size=0.7 r=0.38 turn=21/25
zone ring5 treasure rich r=0.17 turn=9/10
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 pocket1a weak
link p1 pocket1b weak
link p2 pocket2a weak
link p2 pocket2b weak
link p3 pocket3a weak
link p3 pocket3b weak
link p4 pocket4a weak
link p4 pocket4b weak
link p5 pocket5a weak
link p5 pocket5b weak
link p1 ring1 normal
link p2 ring1 normal
link p2 ring2 normal
link p3 ring2 normal
link p3 ring3 normal
link p4 ring3 normal
link p4 ring4 normal
link p5 ring4 normal
link p5 ring5 normal
link p1 ring5 normal
link ring1 middle strong
link ring2 middle strong
link ring3 middle strong
link ring4 middle strong
link ring5 middle strong
)tpl"
     R"tpl(
name Pockets
players 6
sizes medium

zone p1 start size=1.1 r=0.3 turn=0
zone pocket1a treasure size=0.7 r=0.38 turn=29/30
zone pocket1b treasure size=0.7 r=0.38 turn=1/30
zone ring1 treasure r=0.17 turn=1/12
zone p2 start size=1.1 r=0.3 turn=1/6
zone pocket2a treasure size=0.7 r=0.38 turn=2/15
zone pocket2b treasure size=0.7 r=0.38 turn=1/5
zone ring2 treasure r=0.17 turn=1/4
zone p3 start size=1.1 r=0.3 turn=1/3
zone pocket3a treasure size=0.7 r=0.38 turn=3/10
zone pocket3b treasure size=0.7 r=0.38 turn=11/30
zone ring3 treasure r=0.17 turn=5/12
zone p4 start size=1.1 r=0.3 turn=1/2
zone pocket4a treasure size=0.7 r=0.38 turn=7/15
zone pocket4b treasure size=0.7 r=0.38 turn=8/15
zone ring4 treasure r=0.17 turn=7/12
zone p5 start size=1.1 r=0.3 turn=2/3
zone pocket5a treasure size=0.7 r=0.38 turn=19/30
zone pocket5b treasure size=0.7 r=0.38 turn=7/10
zone ring5 treasure r=0.17 turn=3/4
zone p6 start size=1.1 r=0.3 turn=5/6
zone pocket6a treasure size=0.7 r=0.38 turn=4/5
zone pocket6b treasure size=0.7 r=0.38 turn=13/15
zone ring6 treasure r=0.17 turn=11/12
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 pocket1a weak
link p1 pocket1b weak
link p2 pocket2a weak
link p2 pocket2b weak
link p3 pocket3a weak
link p3 pocket3b weak
link p4 pocket4a weak
link p4 pocket4b weak
link p5 pocket5a weak
link p5 pocket5b weak
link p6 pocket6a weak
link p6 pocket6b weak
link p1 ring1 normal
link p2 ring1 normal
link p2 ring2 normal
link p3 ring2 normal
link p3 ring3 normal
link p4 ring3 normal
link p4 ring4 normal
link p5 ring4 normal
link p5 ring5 normal
link p6 ring5 normal
link p6 ring6 normal
link p1 ring6 normal
link ring1 middle strong
link ring2 middle strong
link ring3 middle strong
link ring4 middle strong
link ring5 middle strong
link ring6 middle strong
)tpl"
     R"tpl(
name Pockets
players 6
sizes large-huge

zone p1 start size=1.1 r=0.3 turn=0
zone pocket1a treasure size=0.7 r=0.38 turn=29/30
zone pocket1b treasure size=0.7 r=0.38 turn=1/30
zone ring1 treasure rich r=0.17 turn=1/12
zone p2 start size=1.1 r=0.3 turn=1/6
zone pocket2a treasure size=0.7 r=0.38 turn=2/15
zone pocket2b treasure size=0.7 r=0.38 turn=1/5
zone ring2 treasure rich r=0.17 turn=1/4
zone p3 start size=1.1 r=0.3 turn=1/3
zone pocket3a treasure size=0.7 r=0.38 turn=3/10
zone pocket3b treasure size=0.7 r=0.38 turn=11/30
zone ring3 treasure rich r=0.17 turn=5/12
zone p4 start size=1.1 r=0.3 turn=1/2
zone pocket4a treasure size=0.7 r=0.38 turn=7/15
zone pocket4b treasure size=0.7 r=0.38 turn=8/15
zone ring4 treasure rich r=0.17 turn=7/12
zone p5 start size=1.1 r=0.3 turn=2/3
zone pocket5a treasure size=0.7 r=0.38 turn=19/30
zone pocket5b treasure size=0.7 r=0.38 turn=7/10
zone ring5 treasure rich r=0.17 turn=3/4
zone p6 start size=1.1 r=0.3 turn=5/6
zone pocket6a treasure size=0.7 r=0.38 turn=4/5
zone pocket6b treasure size=0.7 r=0.38 turn=13/15
zone ring6 treasure rich r=0.17 turn=11/12
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 pocket1a weak
link p1 pocket1b weak
link p2 pocket2a weak
link p2 pocket2b weak
link p3 pocket3a weak
link p3 pocket3b weak
link p4 pocket4a weak
link p4 pocket4b weak
link p5 pocket5a weak
link p5 pocket5b weak
link p6 pocket6a weak
link p6 pocket6b weak
link p1 ring1 normal
link p2 ring1 normal
link p2 ring2 normal
link p3 ring2 normal
link p3 ring3 normal
link p4 ring3 normal
link p4 ring4 normal
link p5 ring4 normal
link p5 ring5 normal
link p6 ring5 normal
link p6 ring6 normal
link p1 ring6 normal
link ring1 middle strong
link ring2 middle strong
link ring3 middle strong
link ring4 middle strong
link ring5 middle strong
link ring6 middle strong
)tpl"},
    {"race.txt",
     R"tpl(# Race: each player a lane of richer zones to a deadly guarded middle with a town.
)tpl"
     R"tpl(
name Race
players 2
sizes small-medium

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.37 turn=0
zone lane1a treasure r=0.26 turn=0
zone lane1b treasure rich r=0.15 turn=0
zone p2 start r=0.37 turn=1/2
zone lane2a treasure r=0.26 turn=1/2
zone lane2b treasure rich r=0.15 turn=1/2
link p1 lane1a weak
link lane1a lane1b strong
link lane1b middle deadly
link p2 lane2a weak
link lane2a lane2b strong
link lane2b middle deadly
link lane1a lane2a strong
)tpl"
     R"tpl(
name Race
players 2
sizes large-huge

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.37 turn=0
zone lane1a treasure r=0.29 turn=0
zone lane1b treasure r=0.21 turn=0
zone lane1c treasure rich r=0.13 turn=0
zone p2 start r=0.37 turn=1/2
zone lane2a treasure r=0.29 turn=1/2
zone lane2b treasure r=0.21 turn=1/2
zone lane2c treasure rich r=0.13 turn=1/2
link p1 lane1a weak
link lane1a lane1b normal
link lane1b lane1c strong
link lane1c middle deadly
link p2 lane2a weak
link lane2a lane2b normal
link lane2b lane2c strong
link lane2c middle deadly
link lane1a lane2a strong
)tpl"
     R"tpl(
name Race
players 3
sizes small-medium

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.37 turn=0
zone lane1a treasure r=0.26 turn=0
zone lane1b treasure rich r=0.15 turn=0
zone p2 start r=0.37 turn=1/3
zone lane2a treasure r=0.26 turn=1/3
zone lane2b treasure rich r=0.15 turn=1/3
zone p3 start r=0.37 turn=2/3
zone lane3a treasure r=0.26 turn=2/3
zone lane3b treasure rich r=0.15 turn=2/3
link p1 lane1a weak
link lane1a lane1b strong
link lane1b middle deadly
link p2 lane2a weak
link lane2a lane2b strong
link lane2b middle deadly
link p3 lane3a weak
link lane3a lane3b strong
link lane3b middle deadly
link lane1a lane2a strong
link lane2a lane3a strong
link lane3a lane1a strong
)tpl"
     R"tpl(
name Race
players 3
sizes large-huge

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.37 turn=0
zone lane1a treasure r=0.29 turn=0
zone lane1b treasure r=0.21 turn=0
zone lane1c treasure rich r=0.13 turn=0
zone p2 start r=0.37 turn=1/3
zone lane2a treasure r=0.29 turn=1/3
zone lane2b treasure r=0.21 turn=1/3
zone lane2c treasure rich r=0.13 turn=1/3
zone p3 start r=0.37 turn=2/3
zone lane3a treasure r=0.29 turn=2/3
zone lane3b treasure r=0.21 turn=2/3
zone lane3c treasure rich r=0.13 turn=2/3
link p1 lane1a weak
link lane1a lane1b normal
link lane1b lane1c strong
link lane1c middle deadly
link p2 lane2a weak
link lane2a lane2b normal
link lane2b lane2c strong
link lane2c middle deadly
link p3 lane3a weak
link lane3a lane3b normal
link lane3b lane3c strong
link lane3c middle deadly
link lane1a lane2a strong
link lane2a lane3a strong
link lane3a lane1a strong
)tpl"
     R"tpl(
name Race
players 4
sizes small-medium

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.37 turn=0
zone lane1a treasure r=0.26 turn=0
zone lane1b treasure rich r=0.15 turn=0
zone p2 start r=0.37 turn=1/4
zone lane2a treasure r=0.26 turn=1/4
zone lane2b treasure rich r=0.15 turn=1/4
zone p3 start r=0.37 turn=1/2
zone lane3a treasure r=0.26 turn=1/2
zone lane3b treasure rich r=0.15 turn=1/2
zone p4 start r=0.37 turn=3/4
zone lane4a treasure r=0.26 turn=3/4
zone lane4b treasure rich r=0.15 turn=3/4
link p1 lane1a weak
link lane1a lane1b strong
link lane1b middle deadly
link p2 lane2a weak
link lane2a lane2b strong
link lane2b middle deadly
link p3 lane3a weak
link lane3a lane3b strong
link lane3b middle deadly
link p4 lane4a weak
link lane4a lane4b strong
link lane4b middle deadly
link lane1a lane2a strong
link lane2a lane3a strong
link lane3a lane4a strong
link lane4a lane1a strong
)tpl"
     R"tpl(
name Race
players 4
sizes large-huge

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.37 turn=0
zone lane1a treasure r=0.29 turn=0
zone lane1b treasure r=0.21 turn=0
zone lane1c treasure rich r=0.13 turn=0
zone p2 start r=0.37 turn=1/4
zone lane2a treasure r=0.29 turn=1/4
zone lane2b treasure r=0.21 turn=1/4
zone lane2c treasure rich r=0.13 turn=1/4
zone p3 start r=0.37 turn=1/2
zone lane3a treasure r=0.29 turn=1/2
zone lane3b treasure r=0.21 turn=1/2
zone lane3c treasure rich r=0.13 turn=1/2
zone p4 start r=0.37 turn=3/4
zone lane4a treasure r=0.29 turn=3/4
zone lane4b treasure r=0.21 turn=3/4
zone lane4c treasure rich r=0.13 turn=3/4
link p1 lane1a weak
link lane1a lane1b normal
link lane1b lane1c strong
link lane1c middle deadly
link p2 lane2a weak
link lane2a lane2b normal
link lane2b lane2c strong
link lane2c middle deadly
link p3 lane3a weak
link lane3a lane3b normal
link lane3b lane3c strong
link lane3c middle deadly
link p4 lane4a weak
link lane4a lane4b normal
link lane4b lane4c strong
link lane4c middle deadly
link lane1a lane2a strong
link lane2a lane3a strong
link lane3a lane4a strong
link lane4a lane1a strong
)tpl"
     R"tpl(
name Race
players 5
sizes medium

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.37 turn=0
zone lane1a treasure r=0.26 turn=0
zone lane1b treasure rich r=0.15 turn=0
zone p2 start r=0.37 turn=1/5
zone lane2a treasure r=0.26 turn=1/5
zone lane2b treasure rich r=0.15 turn=1/5
zone p3 start r=0.37 turn=2/5
zone lane3a treasure r=0.26 turn=2/5
zone lane3b treasure rich r=0.15 turn=2/5
zone p4 start r=0.37 turn=3/5
zone lane4a treasure r=0.26 turn=3/5
zone lane4b treasure rich r=0.15 turn=3/5
zone p5 start r=0.37 turn=4/5
zone lane5a treasure r=0.26 turn=4/5
zone lane5b treasure rich r=0.15 turn=4/5
link p1 lane1a weak
link lane1a lane1b strong
link lane1b middle deadly
link p2 lane2a weak
link lane2a lane2b strong
link lane2b middle deadly
link p3 lane3a weak
link lane3a lane3b strong
link lane3b middle deadly
link p4 lane4a weak
link lane4a lane4b strong
link lane4b middle deadly
link p5 lane5a weak
link lane5a lane5b strong
link lane5b middle deadly
link lane1a lane2a strong
link lane2a lane3a strong
link lane3a lane4a strong
link lane4a lane5a strong
link lane5a lane1a strong
)tpl"
     R"tpl(
name Race
players 5
sizes large-huge

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.37 turn=0
zone lane1a treasure r=0.29 turn=0
zone lane1b treasure r=0.21 turn=0
zone lane1c treasure rich r=0.13 turn=0
zone p2 start r=0.37 turn=1/5
zone lane2a treasure r=0.29 turn=1/5
zone lane2b treasure r=0.21 turn=1/5
zone lane2c treasure rich r=0.13 turn=1/5
zone p3 start r=0.37 turn=2/5
zone lane3a treasure r=0.29 turn=2/5
zone lane3b treasure r=0.21 turn=2/5
zone lane3c treasure rich r=0.13 turn=2/5
zone p4 start r=0.37 turn=3/5
zone lane4a treasure r=0.29 turn=3/5
zone lane4b treasure r=0.21 turn=3/5
zone lane4c treasure rich r=0.13 turn=3/5
zone p5 start r=0.37 turn=4/5
zone lane5a treasure r=0.29 turn=4/5
zone lane5b treasure r=0.21 turn=4/5
zone lane5c treasure rich r=0.13 turn=4/5
link p1 lane1a weak
link lane1a lane1b normal
link lane1b lane1c strong
link lane1c middle deadly
link p2 lane2a weak
link lane2a lane2b normal
link lane2b lane2c strong
link lane2c middle deadly
link p3 lane3a weak
link lane3a lane3b normal
link lane3b lane3c strong
link lane3c middle deadly
link p4 lane4a weak
link lane4a lane4b normal
link lane4b lane4c strong
link lane4c middle deadly
link p5 lane5a weak
link lane5a lane5b normal
link lane5b lane5c strong
link lane5c middle deadly
link lane1a lane2a strong
link lane2a lane3a strong
link lane3a lane4a strong
link lane4a lane5a strong
link lane5a lane1a strong
)tpl"
     R"tpl(
name Race
players 6
sizes medium

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.37 turn=0
zone lane1a treasure r=0.26 turn=0
zone lane1b treasure rich r=0.15 turn=0
zone p2 start r=0.37 turn=1/6
zone lane2a treasure r=0.26 turn=1/6
zone lane2b treasure rich r=0.15 turn=1/6
zone p3 start r=0.37 turn=1/3
zone lane3a treasure r=0.26 turn=1/3
zone lane3b treasure rich r=0.15 turn=1/3
zone p4 start r=0.37 turn=1/2
zone lane4a treasure r=0.26 turn=1/2
zone lane4b treasure rich r=0.15 turn=1/2
zone p5 start r=0.37 turn=2/3
zone lane5a treasure r=0.26 turn=2/3
zone lane5b treasure rich r=0.15 turn=2/3
zone p6 start r=0.37 turn=5/6
zone lane6a treasure r=0.26 turn=5/6
zone lane6b treasure rich r=0.15 turn=5/6
link p1 lane1a weak
link lane1a lane1b strong
link lane1b middle deadly
link p2 lane2a weak
link lane2a lane2b strong
link lane2b middle deadly
link p3 lane3a weak
link lane3a lane3b strong
link lane3b middle deadly
link p4 lane4a weak
link lane4a lane4b strong
link lane4b middle deadly
link p5 lane5a weak
link lane5a lane5b strong
link lane5b middle deadly
link p6 lane6a weak
link lane6a lane6b strong
link lane6b middle deadly
link lane1a lane2a strong
link lane2a lane3a strong
link lane3a lane4a strong
link lane4a lane5a strong
link lane5a lane6a strong
link lane6a lane1a strong
)tpl"
     R"tpl(
name Race
players 6
sizes large-huge

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.37 turn=0
zone lane1a treasure r=0.29 turn=0
zone lane1b treasure r=0.21 turn=0
zone lane1c treasure rich r=0.13 turn=0
zone p2 start r=0.37 turn=1/6
zone lane2a treasure r=0.29 turn=1/6
zone lane2b treasure r=0.21 turn=1/6
zone lane2c treasure rich r=0.13 turn=1/6
zone p3 start r=0.37 turn=1/3
zone lane3a treasure r=0.29 turn=1/3
zone lane3b treasure r=0.21 turn=1/3
zone lane3c treasure rich r=0.13 turn=1/3
zone p4 start r=0.37 turn=1/2
zone lane4a treasure r=0.29 turn=1/2
zone lane4b treasure r=0.21 turn=1/2
zone lane4c treasure rich r=0.13 turn=1/2
zone p5 start r=0.37 turn=2/3
zone lane5a treasure r=0.29 turn=2/3
zone lane5b treasure r=0.21 turn=2/3
zone lane5c treasure rich r=0.13 turn=2/3
zone p6 start r=0.37 turn=5/6
zone lane6a treasure r=0.29 turn=5/6
zone lane6b treasure r=0.21 turn=5/6
zone lane6c treasure rich r=0.13 turn=5/6
link p1 lane1a weak
link lane1a lane1b normal
link lane1b lane1c strong
link lane1c middle deadly
link p2 lane2a weak
link lane2a lane2b normal
link lane2b lane2c strong
link lane2c middle deadly
link p3 lane3a weak
link lane3a lane3b normal
link lane3b lane3c strong
link lane3c middle deadly
link p4 lane4a weak
link lane4a lane4b normal
link lane4b lane4c strong
link lane4c middle deadly
link p5 lane5a weak
link lane5a lane5b normal
link lane5b lane5c strong
link lane5c middle deadly
link p6 lane6a weak
link lane6a lane6b normal
link lane6b lane6c strong
link lane6c middle deadly
link lane1a lane2a strong
link lane2a lane3a strong
link lane3a lane4a strong
link lane4a lane5a strong
link lane5a lane6a strong
link lane6a lane1a strong
)tpl"},
    {"ring.txt",
     R"tpl(# Ring: the players' starts round the map, treasure zones between neighbours (the richer in
# each arc's middle); on large maps a rich middle with a town behind deadly guards.
# One variant for each number of players and the map sizes that share a layout.
)tpl"
     R"tpl(
name Ring
players 2
sizes small

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/2
zone arc1 treasure r=0.37 turn=1/4
zone arc2 treasure r=0.37 turn=3/4
link p1 arc1
link arc1 p2
link p2 arc2
link arc2 p1
)tpl"
     R"tpl(
name Ring
players 2
sizes medium

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/2
zone arc1 treasure r=0.37 turn=1/6
zone arc2 treasure rich r=0.37 turn=1/3
zone arc3 treasure r=0.37 turn=2/3
zone arc4 treasure rich r=0.37 turn=5/6
link p1 arc1
link arc1 arc2
link arc2 p2
link p2 arc3
link arc3 arc4
link arc4 p1
)tpl"
     R"tpl(
name Ring
players 2
sizes large

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/2
zone centre1 treasure rich town size=1.5 x=0 y=0
zone arc1 treasure r=0.37 turn=1/6
zone arc2 treasure rich r=0.37 turn=1/3
zone arc3 treasure r=0.37 turn=2/3
zone arc4 treasure rich r=0.37 turn=5/6
link p1 arc1
link arc1 arc2
link arc2 centre1 deadly
link arc2 p2
link p2 arc3
link arc3 arc4
link arc4 centre1 deadly
link arc4 p1
)tpl"
     R"tpl(
name Ring
players 2
sizes huge

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/2
zone centre1 treasure rich town size=1.5 x=0 y=0
zone arc1 treasure r=0.37 turn=1/8
zone arc2 treasure rich r=0.37 turn=1/4
zone arc3 treasure r=0.37 turn=3/8
zone arc4 treasure r=0.37 turn=5/8
zone arc5 treasure rich r=0.37 turn=3/4
zone arc6 treasure r=0.37 turn=7/8
link p1 arc1
link arc1 arc2
link arc2 centre1 deadly
link arc2 arc3
link arc3 p2
link p2 arc4
link arc4 arc5
link arc5 centre1 deadly
link arc5 arc6
link arc6 p1
)tpl"
     R"tpl(
name Ring
players 3
sizes small

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/3
zone p3 start size=1.2 r=0.36 turn=2/3
zone arc1 treasure r=0.37 turn=1/6
zone arc2 treasure r=0.37 turn=1/2
zone arc3 treasure r=0.37 turn=5/6
link p1 arc1
link arc1 p2
link p2 arc2
link arc2 p3
link p3 arc3
link arc3 p1
)tpl"
     R"tpl(
name Ring
players 3
sizes medium

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/3
zone p3 start size=1.2 r=0.36 turn=2/3
zone arc1 treasure r=0.37 turn=1/9
zone arc2 treasure rich r=0.37 turn=2/9
zone arc3 treasure r=0.37 turn=4/9
zone arc4 treasure rich r=0.37 turn=5/9
zone arc5 treasure r=0.37 turn=7/9
zone arc6 treasure rich r=0.37 turn=8/9
link p1 arc1
link arc1 arc2
link arc2 p2
link p2 arc3
link arc3 arc4
link arc4 p3
link p3 arc5
link arc5 arc6
link arc6 p1
)tpl"
     R"tpl(
name Ring
players 3
sizes large

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/3
zone p3 start size=1.2 r=0.36 turn=2/3
zone centre1 treasure rich town size=1.5 x=0 y=0
zone arc1 treasure r=0.37 turn=1/9
zone arc2 treasure rich r=0.37 turn=2/9
zone arc3 treasure r=0.37 turn=4/9
zone arc4 treasure rich r=0.37 turn=5/9
zone arc5 treasure r=0.37 turn=7/9
zone arc6 treasure rich r=0.37 turn=8/9
link p1 arc1
link arc1 arc2
link arc2 centre1 deadly
link arc2 p2
link p2 arc3
link arc3 arc4
link arc4 centre1 deadly
link arc4 p3
link p3 arc5
link arc5 arc6
link arc6 centre1 deadly
link arc6 p1
)tpl"
     R"tpl(
name Ring
players 3
sizes huge

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/3
zone p3 start size=1.2 r=0.36 turn=2/3
zone centre1 treasure rich town size=1.5 x=0 y=0
zone arc1 treasure r=0.37 turn=1/12
zone arc2 treasure rich r=0.37 turn=1/6
zone arc3 treasure r=0.37 turn=1/4
zone arc4 treasure r=0.37 turn=5/12
zone arc5 treasure rich r=0.37 turn=1/2
zone arc6 treasure r=0.37 turn=7/12
zone arc7 treasure r=0.37 turn=3/4
zone arc8 treasure rich r=0.37 turn=5/6
zone arc9 treasure r=0.37 turn=11/12
link p1 arc1
link arc1 arc2
link arc2 centre1 deadly
link arc2 arc3
link arc3 p2
link p2 arc4
link arc4 arc5
link arc5 centre1 deadly
link arc5 arc6
link arc6 p3
link p3 arc7
link arc7 arc8
link arc8 centre1 deadly
link arc8 arc9
link arc9 p1
)tpl"
     R"tpl(
name Ring
players 4
sizes small

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/4
zone p3 start size=1.2 r=0.36 turn=1/2
zone p4 start size=1.2 r=0.36 turn=3/4
zone arc1 treasure r=0.37 turn=1/8
zone arc2 treasure r=0.37 turn=3/8
zone arc3 treasure r=0.37 turn=5/8
zone arc4 treasure r=0.37 turn=7/8
link p1 arc1
link arc1 p2
link p2 arc2
link arc2 p3
link p3 arc3
link arc3 p4
link p4 arc4
link arc4 p1
)tpl"
     R"tpl(
name Ring
players 4
sizes medium

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/4
zone p3 start size=1.2 r=0.36 turn=1/2
zone p4 start size=1.2 r=0.36 turn=3/4
zone arc1 treasure r=0.37 turn=1/12
zone arc2 treasure rich r=0.37 turn=1/6
zone arc3 treasure r=0.37 turn=1/3
zone arc4 treasure rich r=0.37 turn=5/12
zone arc5 treasure r=0.37 turn=7/12
zone arc6 treasure rich r=0.37 turn=2/3
zone arc7 treasure r=0.37 turn=5/6
zone arc8 treasure rich r=0.37 turn=11/12
link p1 arc1
link arc1 arc2
link arc2 p2
link p2 arc3
link arc3 arc4
link arc4 p3
link p3 arc5
link arc5 arc6
link arc6 p4
link p4 arc7
link arc7 arc8
link arc8 p1
)tpl"
     R"tpl(
name Ring
players 4
sizes large

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/4
zone p3 start size=1.2 r=0.36 turn=1/2
zone p4 start size=1.2 r=0.36 turn=3/4
zone centre1 treasure rich town size=1.5 x=0 y=0
zone arc1 treasure r=0.37 turn=1/12
zone arc2 treasure rich r=0.37 turn=1/6
zone arc3 treasure r=0.37 turn=1/3
zone arc4 treasure rich r=0.37 turn=5/12
zone arc5 treasure r=0.37 turn=7/12
zone arc6 treasure rich r=0.37 turn=2/3
zone arc7 treasure r=0.37 turn=5/6
zone arc8 treasure rich r=0.37 turn=11/12
link p1 arc1
link arc1 arc2
link arc2 centre1 deadly
link arc2 p2
link p2 arc3
link arc3 arc4
link arc4 centre1 deadly
link arc4 p3
link p3 arc5
link arc5 arc6
link arc6 centre1 deadly
link arc6 p4
link p4 arc7
link arc7 arc8
link arc8 centre1 deadly
link arc8 p1
)tpl"
     R"tpl(
name Ring
players 4
sizes huge

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/4
zone p3 start size=1.2 r=0.36 turn=1/2
zone p4 start size=1.2 r=0.36 turn=3/4
zone centre1 treasure rich town size=1.5 x=0 y=0
zone arc1 treasure r=0.37 turn=1/16
zone arc2 treasure rich r=0.37 turn=1/8
zone arc3 treasure r=0.37 turn=3/16
zone arc4 treasure r=0.37 turn=5/16
zone arc5 treasure rich r=0.37 turn=3/8
zone arc6 treasure r=0.37 turn=7/16
zone arc7 treasure r=0.37 turn=9/16
zone arc8 treasure rich r=0.37 turn=5/8
zone arc9 treasure r=0.37 turn=11/16
zone arc10 treasure r=0.37 turn=13/16
zone arc11 treasure rich r=0.37 turn=7/8
zone arc12 treasure r=0.37 turn=15/16
link p1 arc1
link arc1 arc2
link arc2 centre1 deadly
link arc2 arc3
link arc3 p2
link p2 arc4
link arc4 arc5
link arc5 centre1 deadly
link arc5 arc6
link arc6 p3
link p3 arc7
link arc7 arc8
link arc8 centre1 deadly
link arc8 arc9
link arc9 p4
link p4 arc10
link arc10 arc11
link arc11 centre1 deadly
link arc11 arc12
link arc12 p1
)tpl"
     R"tpl(
name Ring
players 5
sizes small-medium

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/5
zone p3 start size=1.2 r=0.36 turn=2/5
zone p4 start size=1.2 r=0.36 turn=3/5
zone p5 start size=1.2 r=0.36 turn=4/5
zone arc1 treasure r=0.37 turn=1/10
zone arc2 treasure r=0.37 turn=3/10
zone arc3 treasure r=0.37 turn=1/2
zone arc4 treasure r=0.37 turn=7/10
zone arc5 treasure r=0.37 turn=9/10
link p1 arc1
link arc1 p2
link p2 arc2
link arc2 p3
link p3 arc3
link arc3 p4
link p4 arc4
link arc4 p5
link p5 arc5
link arc5 p1
)tpl"
     R"tpl(
name Ring
players 5
sizes large

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/5
zone p3 start size=1.2 r=0.36 turn=2/5
zone p4 start size=1.2 r=0.36 turn=3/5
zone p5 start size=1.2 r=0.36 turn=4/5
zone centre1 treasure rich town size=1.5 x=0 y=0
zone arc1 treasure r=0.37 turn=1/15
zone arc2 treasure rich r=0.37 turn=2/15
zone arc3 treasure r=0.37 turn=4/15
zone arc4 treasure rich r=0.37 turn=1/3
zone arc5 treasure r=0.37 turn=7/15
zone arc6 treasure rich r=0.37 turn=8/15
zone arc7 treasure r=0.37 turn=2/3
zone arc8 treasure rich r=0.37 turn=11/15
zone arc9 treasure r=0.37 turn=13/15
zone arc10 treasure rich r=0.37 turn=14/15
link p1 arc1
link arc1 arc2
link arc2 centre1 deadly
link arc2 p2
link p2 arc3
link arc3 arc4
link arc4 centre1 deadly
link arc4 p3
link p3 arc5
link arc5 arc6
link arc6 centre1 deadly
link arc6 p4
link p4 arc7
link arc7 arc8
link arc8 centre1 deadly
link arc8 p5
link p5 arc9
link arc9 arc10
link arc10 centre1 deadly
link arc10 p1
)tpl"
     R"tpl(
name Ring
players 5
sizes huge

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/5
zone p3 start size=1.2 r=0.36 turn=2/5
zone p4 start size=1.2 r=0.36 turn=3/5
zone p5 start size=1.2 r=0.36 turn=4/5
zone centre1 treasure rich town size=1.5 x=0 y=0
zone arc1 treasure r=0.37 turn=1/20
zone arc2 treasure rich r=0.37 turn=1/10
zone arc3 treasure r=0.37 turn=3/20
zone arc4 treasure r=0.37 turn=1/4
zone arc5 treasure rich r=0.37 turn=3/10
zone arc6 treasure r=0.37 turn=7/20
zone arc7 treasure r=0.37 turn=9/20
zone arc8 treasure rich r=0.37 turn=1/2
zone arc9 treasure r=0.37 turn=11/20
zone arc10 treasure r=0.37 turn=13/20
zone arc11 treasure rich r=0.37 turn=7/10
zone arc12 treasure r=0.37 turn=3/4
zone arc13 treasure r=0.37 turn=17/20
zone arc14 treasure rich r=0.37 turn=9/10
zone arc15 treasure r=0.37 turn=19/20
link p1 arc1
link arc1 arc2
link arc2 centre1 deadly
link arc2 arc3
link arc3 p2
link p2 arc4
link arc4 arc5
link arc5 centre1 deadly
link arc5 arc6
link arc6 p3
link p3 arc7
link arc7 arc8
link arc8 centre1 deadly
link arc8 arc9
link arc9 p4
link p4 arc10
link arc10 arc11
link arc11 centre1 deadly
link arc11 arc12
link arc12 p5
link p5 arc13
link arc13 arc14
link arc14 centre1 deadly
link arc14 arc15
link arc15 p1
)tpl"
     R"tpl(
name Ring
players 6
sizes small-medium

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/6
zone p3 start size=1.2 r=0.36 turn=1/3
zone p4 start size=1.2 r=0.36 turn=1/2
zone p5 start size=1.2 r=0.36 turn=2/3
zone p6 start size=1.2 r=0.36 turn=5/6
zone arc1 treasure r=0.37 turn=1/12
zone arc2 treasure r=0.37 turn=1/4
zone arc3 treasure r=0.37 turn=5/12
zone arc4 treasure r=0.37 turn=7/12
zone arc5 treasure r=0.37 turn=3/4
zone arc6 treasure r=0.37 turn=11/12
link p1 arc1
link arc1 p2
link p2 arc2
link arc2 p3
link p3 arc3
link arc3 p4
link p4 arc4
link arc4 p5
link p5 arc5
link arc5 p6
link p6 arc6
link arc6 p1
)tpl"
     R"tpl(
name Ring
players 6
sizes large

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/6
zone p3 start size=1.2 r=0.36 turn=1/3
zone p4 start size=1.2 r=0.36 turn=1/2
zone p5 start size=1.2 r=0.36 turn=2/3
zone p6 start size=1.2 r=0.36 turn=5/6
zone centre1 treasure rich town size=1.5 x=0 y=0
zone arc1 treasure r=0.37 turn=1/18
zone arc2 treasure rich r=0.37 turn=1/9
zone arc3 treasure r=0.37 turn=2/9
zone arc4 treasure rich r=0.37 turn=5/18
zone arc5 treasure r=0.37 turn=7/18
zone arc6 treasure rich r=0.37 turn=4/9
zone arc7 treasure r=0.37 turn=5/9
zone arc8 treasure rich r=0.37 turn=11/18
zone arc9 treasure r=0.37 turn=13/18
zone arc10 treasure rich r=0.37 turn=7/9
zone arc11 treasure r=0.37 turn=8/9
zone arc12 treasure rich r=0.37 turn=17/18
link p1 arc1
link arc1 arc2
link arc2 centre1 deadly
link arc2 p2
link p2 arc3
link arc3 arc4
link arc4 centre1 deadly
link arc4 p3
link p3 arc5
link arc5 arc6
link arc6 centre1 deadly
link arc6 p4
link p4 arc7
link arc7 arc8
link arc8 centre1 deadly
link arc8 p5
link p5 arc9
link arc9 arc10
link arc10 centre1 deadly
link arc10 p6
link p6 arc11
link arc11 arc12
link arc12 centre1 deadly
link arc12 p1
)tpl"
     R"tpl(
name Ring
players 6
sizes huge

zone p1 start size=1.2 r=0.36 turn=0
zone p2 start size=1.2 r=0.36 turn=1/6
zone p3 start size=1.2 r=0.36 turn=1/3
zone p4 start size=1.2 r=0.36 turn=1/2
zone p5 start size=1.2 r=0.36 turn=2/3
zone p6 start size=1.2 r=0.36 turn=5/6
zone centre1 treasure rich town size=1.5 x=0 y=0
zone arc1 treasure r=0.37 turn=1/24
zone arc2 treasure rich r=0.37 turn=1/12
zone arc3 treasure r=0.37 turn=1/8
zone arc4 treasure r=0.37 turn=5/24
zone arc5 treasure rich r=0.37 turn=1/4
zone arc6 treasure r=0.37 turn=7/24
zone arc7 treasure r=0.37 turn=3/8
zone arc8 treasure rich r=0.37 turn=5/12
zone arc9 treasure r=0.37 turn=11/24
zone arc10 treasure r=0.37 turn=13/24
zone arc11 treasure rich r=0.37 turn=7/12
zone arc12 treasure r=0.37 turn=5/8
zone arc13 treasure r=0.37 turn=17/24
zone arc14 treasure rich r=0.37 turn=3/4
zone arc15 treasure r=0.37 turn=19/24
zone arc16 treasure r=0.37 turn=7/8
zone arc17 treasure rich r=0.37 turn=11/12
zone arc18 treasure r=0.37 turn=23/24
link p1 arc1
link arc1 arc2
link arc2 centre1 deadly
link arc2 arc3
link arc3 p2
link p2 arc4
link arc4 arc5
link arc5 centre1 deadly
link arc5 arc6
link arc6 p3
link p3 arc7
link arc7 arc8
link arc8 centre1 deadly
link arc8 arc9
link arc9 p4
link p4 arc10
link arc10 arc11
link arc11 centre1 deadly
link arc11 arc12
link arc12 p5
link p5 arc13
link arc13 arc14
link arc14 centre1 deadly
link arc14 arc15
link arc15 p6
link p6 arc16
link arc16 arc17
link arc17 centre1 deadly
link arc17 arc18
link arc18 p1
)tpl"},
    {"spider.txt",
     R"tpl(# Spider: each start a leg to a rich middle with a town; rich zones between the legs.
)tpl"
     R"tpl(
name Spider
players 2
sizes small-medium

zone middle treasure rich town size=1.3 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=0
zone leg1 treasure r=0.21 turn=0
zone p2 start size=1.2 r=0.37 turn=1/2
zone leg2 treasure r=0.21 turn=1/2
zone web1 treasure rich r=0.33 turn=1/4
zone web2 treasure rich r=0.33 turn=3/4
link p1 leg1 weak
link leg1 middle strong
link p2 leg2 weak
link leg2 middle strong
link leg1 web1 normal
link leg2 web1 normal
link leg2 web2 normal
link leg1 web2 normal
)tpl"
     R"tpl(
name Spider
players 2
sizes large-huge

zone middle treasure rich town size=1.3 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=0
zone leg1 treasure r=0.21 turn=0
zone out1 treasure r=0.4 turn=1/8
zone p2 start size=1.2 r=0.37 turn=1/2
zone leg2 treasure r=0.21 turn=1/2
zone out2 treasure r=0.4 turn=5/8
zone web1 treasure rich r=0.33 turn=1/4
zone web2 treasure rich r=0.33 turn=3/4
link p1 leg1 weak
link leg1 middle strong
link p1 out1 weak
link p2 leg2 weak
link leg2 middle strong
link p2 out2 weak
link leg1 web1 normal
link leg2 web1 normal
link leg2 web2 normal
link leg1 web2 normal
)tpl"
     R"tpl(
name Spider
players 3
sizes small-medium

zone middle treasure rich town size=1.3 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=0
zone leg1 treasure r=0.21 turn=0
zone p2 start size=1.2 r=0.37 turn=1/3
zone leg2 treasure r=0.21 turn=1/3
zone p3 start size=1.2 r=0.37 turn=2/3
zone leg3 treasure r=0.21 turn=2/3
zone web1 treasure rich r=0.33 turn=1/6
zone web2 treasure rich r=0.33 turn=1/2
zone web3 treasure rich r=0.33 turn=5/6
link p1 leg1 weak
link leg1 middle strong
link p2 leg2 weak
link leg2 middle strong
link p3 leg3 weak
link leg3 middle strong
link leg1 web1 normal
link leg2 web1 normal
link leg2 web2 normal
link leg3 web2 normal
link leg3 web3 normal
link leg1 web3 normal
)tpl"
     R"tpl(
name Spider
players 3
sizes large-huge

zone middle treasure rich town size=1.3 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=0
zone leg1 treasure r=0.21 turn=0
zone out1 treasure r=0.4 turn=1/12
zone p2 start size=1.2 r=0.37 turn=1/3
zone leg2 treasure r=0.21 turn=1/3
zone out2 treasure r=0.4 turn=5/12
zone p3 start size=1.2 r=0.37 turn=2/3
zone leg3 treasure r=0.21 turn=2/3
zone out3 treasure r=0.4 turn=3/4
zone web1 treasure rich r=0.33 turn=1/6
zone web2 treasure rich r=0.33 turn=1/2
zone web3 treasure rich r=0.33 turn=5/6
link p1 leg1 weak
link leg1 middle strong
link p1 out1 weak
link p2 leg2 weak
link leg2 middle strong
link p2 out2 weak
link p3 leg3 weak
link leg3 middle strong
link p3 out3 weak
link leg1 web1 normal
link leg2 web1 normal
link leg2 web2 normal
link leg3 web2 normal
link leg3 web3 normal
link leg1 web3 normal
)tpl"
     R"tpl(
name Spider
players 4
sizes small-medium

zone middle treasure rich town size=1.3 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=0
zone leg1 treasure r=0.21 turn=0
zone p2 start size=1.2 r=0.37 turn=1/4
zone leg2 treasure r=0.21 turn=1/4
zone p3 start size=1.2 r=0.37 turn=1/2
zone leg3 treasure r=0.21 turn=1/2
zone p4 start size=1.2 r=0.37 turn=3/4
zone leg4 treasure r=0.21 turn=3/4
zone web1 treasure rich r=0.33 turn=1/8
zone web2 treasure rich r=0.33 turn=3/8
zone web3 treasure rich r=0.33 turn=5/8
zone web4 treasure rich r=0.33 turn=7/8
link p1 leg1 weak
link leg1 middle strong
link p2 leg2 weak
link leg2 middle strong
link p3 leg3 weak
link leg3 middle strong
link p4 leg4 weak
link leg4 middle strong
link leg1 web1 normal
link leg2 web1 normal
link leg2 web2 normal
link leg3 web2 normal
link leg3 web3 normal
link leg4 web3 normal
link leg4 web4 normal
link leg1 web4 normal
)tpl"
     R"tpl(
name Spider
players 4
sizes large-huge

zone middle treasure rich town size=1.3 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=0
zone leg1 treasure r=0.21 turn=0
zone out1 treasure r=0.4 turn=1/16
zone p2 start size=1.2 r=0.37 turn=1/4
zone leg2 treasure r=0.21 turn=1/4
zone out2 treasure r=0.4 turn=5/16
zone p3 start size=1.2 r=0.37 turn=1/2
zone leg3 treasure r=0.21 turn=1/2
zone out3 treasure r=0.4 turn=9/16
zone p4 start size=1.2 r=0.37 turn=3/4
zone leg4 treasure r=0.21 turn=3/4
zone out4 treasure r=0.4 turn=13/16
zone web1 treasure rich r=0.33 turn=1/8
zone web2 treasure rich r=0.33 turn=3/8
zone web3 treasure rich r=0.33 turn=5/8
zone web4 treasure rich r=0.33 turn=7/8
link p1 leg1 weak
link leg1 middle strong
link p1 out1 weak
link p2 leg2 weak
link leg2 middle strong
link p2 out2 weak
link p3 leg3 weak
link leg3 middle strong
link p3 out3 weak
link p4 leg4 weak
link leg4 middle strong
link p4 out4 weak
link leg1 web1 normal
link leg2 web1 normal
link leg2 web2 normal
link leg3 web2 normal
link leg3 web3 normal
link leg4 web3 normal
link leg4 web4 normal
link leg1 web4 normal
)tpl"
     R"tpl(
name Spider
players 5
sizes medium

zone middle treasure rich town size=1.3 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=0
zone leg1 treasure r=0.21 turn=0
zone p2 start size=1.2 r=0.37 turn=1/5
zone leg2 treasure r=0.21 turn=1/5
zone p3 start size=1.2 r=0.37 turn=2/5
zone leg3 treasure r=0.21 turn=2/5
zone p4 start size=1.2 r=0.37 turn=3/5
zone leg4 treasure r=0.21 turn=3/5
zone p5 start size=1.2 r=0.37 turn=4/5
zone leg5 treasure r=0.21 turn=4/5
zone web1 treasure rich r=0.33 turn=1/10
zone web2 treasure rich r=0.33 turn=3/10
zone web3 treasure rich r=0.33 turn=1/2
zone web4 treasure rich r=0.33 turn=7/10
zone web5 treasure rich r=0.33 turn=9/10
link p1 leg1 weak
link leg1 middle strong
link p2 leg2 weak
link leg2 middle strong
link p3 leg3 weak
link leg3 middle strong
link p4 leg4 weak
link leg4 middle strong
link p5 leg5 weak
link leg5 middle strong
link leg1 web1 normal
link leg2 web1 normal
link leg2 web2 normal
link leg3 web2 normal
link leg3 web3 normal
link leg4 web3 normal
link leg4 web4 normal
link leg5 web4 normal
link leg5 web5 normal
link leg1 web5 normal
)tpl"
     R"tpl(
name Spider
players 5
sizes large-huge

zone middle treasure rich town size=1.3 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=0
zone leg1 treasure r=0.21 turn=0
zone out1 treasure r=0.4 turn=1/20
zone p2 start size=1.2 r=0.37 turn=1/5
zone leg2 treasure r=0.21 turn=1/5
zone out2 treasure r=0.4 turn=1/4
zone p3 start size=1.2 r=0.37 turn=2/5
zone leg3 treasure r=0.21 turn=2/5
zone out3 treasure r=0.4 turn=9/20
zone p4 start size=1.2 r=0.37 turn=3/5
zone leg4 treasure r=0.21 turn=3/5
zone out4 treasure r=0.4 turn=13/20
zone p5 start size=1.2 r=0.37 turn=4/5
zone leg5 treasure r=0.21 turn=4/5
zone out5 treasure r=0.4 turn=17/20
zone web1 treasure rich r=0.33 turn=1/10
zone web2 treasure rich r=0.33 turn=3/10
zone web3 treasure rich r=0.33 turn=1/2
zone web4 treasure rich r=0.33 turn=7/10
zone web5 treasure rich r=0.33 turn=9/10
link p1 leg1 weak
link leg1 middle strong
link p1 out1 weak
link p2 leg2 weak
link leg2 middle strong
link p2 out2 weak
link p3 leg3 weak
link leg3 middle strong
link p3 out3 weak
link p4 leg4 weak
link leg4 middle strong
link p4 out4 weak
link p5 leg5 weak
link leg5 middle strong
link p5 out5 weak
link leg1 web1 normal
link leg2 web1 normal
link leg2 web2 normal
link leg3 web2 normal
link leg3 web3 normal
link leg4 web3 normal
link leg4 web4 normal
link leg5 web4 normal
link leg5 web5 normal
link leg1 web5 normal
)tpl"
     R"tpl(
name Spider
players 6
sizes medium

zone middle treasure rich town size=1.3 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=0
zone leg1 treasure r=0.21 turn=0
zone p2 start size=1.2 r=0.37 turn=1/6
zone leg2 treasure r=0.21 turn=1/6
zone p3 start size=1.2 r=0.37 turn=1/3
zone leg3 treasure r=0.21 turn=1/3
zone p4 start size=1.2 r=0.37 turn=1/2
zone leg4 treasure r=0.21 turn=1/2
zone p5 start size=1.2 r=0.37 turn=2/3
zone leg5 treasure r=0.21 turn=2/3
zone p6 start size=1.2 r=0.37 turn=5/6
zone leg6 treasure r=0.21 turn=5/6
zone web1 treasure rich r=0.33 turn=1/12
zone web2 treasure rich r=0.33 turn=1/4
zone web3 treasure rich r=0.33 turn=5/12
zone web4 treasure rich r=0.33 turn=7/12
zone web5 treasure rich r=0.33 turn=3/4
zone web6 treasure rich r=0.33 turn=11/12
link p1 leg1 weak
link leg1 middle strong
link p2 leg2 weak
link leg2 middle strong
link p3 leg3 weak
link leg3 middle strong
link p4 leg4 weak
link leg4 middle strong
link p5 leg5 weak
link leg5 middle strong
link p6 leg6 weak
link leg6 middle strong
link leg1 web1 normal
link leg2 web1 normal
link leg2 web2 normal
link leg3 web2 normal
link leg3 web3 normal
link leg4 web3 normal
link leg4 web4 normal
link leg5 web4 normal
link leg5 web5 normal
link leg6 web5 normal
link leg6 web6 normal
link leg1 web6 normal
)tpl"
     R"tpl(
name Spider
players 6
sizes large-huge

zone middle treasure rich town size=1.3 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=0
zone leg1 treasure r=0.21 turn=0
zone out1 treasure r=0.4 turn=1/24
zone p2 start size=1.2 r=0.37 turn=1/6
zone leg2 treasure r=0.21 turn=1/6
zone out2 treasure r=0.4 turn=5/24
zone p3 start size=1.2 r=0.37 turn=1/3
zone leg3 treasure r=0.21 turn=1/3
zone out3 treasure r=0.4 turn=3/8
zone p4 start size=1.2 r=0.37 turn=1/2
zone leg4 treasure r=0.21 turn=1/2
zone out4 treasure r=0.4 turn=13/24
zone p5 start size=1.2 r=0.37 turn=2/3
zone leg5 treasure r=0.21 turn=2/3
zone out5 treasure r=0.4 turn=17/24
zone p6 start size=1.2 r=0.37 turn=5/6
zone leg6 treasure r=0.21 turn=5/6
zone out6 treasure r=0.4 turn=7/8
zone web1 treasure rich r=0.33 turn=1/12
zone web2 treasure rich r=0.33 turn=1/4
zone web3 treasure rich r=0.33 turn=5/12
zone web4 treasure rich r=0.33 turn=7/12
zone web5 treasure rich r=0.33 turn=3/4
zone web6 treasure rich r=0.33 turn=11/12
link p1 leg1 weak
link leg1 middle strong
link p1 out1 weak
link p2 leg2 weak
link leg2 middle strong
link p2 out2 weak
link p3 leg3 weak
link leg3 middle strong
link p3 out3 weak
link p4 leg4 weak
link leg4 middle strong
link p4 out4 weak
link p5 leg5 weak
link leg5 middle strong
link p5 out5 weak
link p6 leg6 weak
link leg6 middle strong
link p6 out6 weak
link leg1 web1 normal
link leg2 web1 normal
link leg2 web2 normal
link leg3 web2 normal
link leg3 web3 normal
link leg4 web3 normal
link leg4 web4 normal
link leg5 web4 normal
link leg5 web5 normal
link leg6 web5 normal
link leg6 web6 normal
link leg1 web6 normal
)tpl"},
    {"spiral.txt",
     R"tpl(# Spiral: the starts along a spiral's arm to the rich middle, each nearer it than the last.
)tpl"
     R"tpl(
name Spiral
players 2
sizes small-medium

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.38 turn=0
zone arm2 treasure r=0.313 turn=11/20
zone p2 start r=0.247 turn=1/10
zone arm4 treasure rich r=0.18 turn=13/20
link p1 arm2 normal
link arm2 p2 normal
link p2 arm4 normal
link arm4 middle strong
)tpl"
     R"tpl(
name Spiral
players 2
sizes large-huge

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.38 turn=0
zone arm2 treasure r=0.34 turn=8/15
zone arm3 treasure r=0.3 turn=1/15
zone p2 start r=0.26 turn=3/5
zone arm5 treasure rich r=0.22 turn=2/15
zone arm6 treasure rich r=0.18 turn=2/3
link p1 arm2 normal
link arm2 arm3 normal
link arm3 p2 normal
link p2 arm5 normal
link arm5 arm6 normal
link arm6 middle strong
)tpl"
     R"tpl(
name Spiral
players 3
sizes small-medium

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.38 turn=0
zone arm2 treasure r=0.34 turn=11/30
zone p2 start r=0.3 turn=11/15
zone arm4 treasure r=0.26 turn=1/10
zone p3 start r=0.22 turn=7/15
zone arm6 treasure rich r=0.18 turn=5/6
link p1 arm2 normal
link arm2 p2 normal
link p2 arm4 normal
link arm4 p3 normal
link p3 arm6 normal
link arm6 middle strong
)tpl"
     R"tpl(
name Spiral
players 3
sizes large-huge

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.38 turn=0
zone arm2 treasure r=0.355 turn=16/45
zone arm3 treasure r=0.33 turn=32/45
zone p2 start r=0.305 turn=1/15
zone arm5 treasure r=0.28 turn=19/45
zone arm6 treasure r=0.255 turn=7/9
zone p3 start r=0.23 turn=2/15
zone arm8 treasure rich r=0.205 turn=22/45
zone arm9 treasure rich r=0.18 turn=38/45
link p1 arm2 normal
link arm2 arm3 normal
link arm3 p2 normal
link p2 arm5 normal
link arm5 arm6 normal
link arm6 p3 normal
link p3 arm8 normal
link arm8 arm9 normal
link arm9 middle strong
)tpl"
     R"tpl(
name Spiral
players 4
sizes small-medium

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.38 turn=0
zone arm2 treasure r=0.351 turn=11/40
zone p2 start r=0.323 turn=11/20
zone arm4 treasure r=0.294 turn=33/40
zone p3 start r=0.266 turn=1/10
zone arm6 treasure rich r=0.237 turn=3/8
zone p4 start r=0.209 turn=13/20
zone arm8 treasure rich r=0.18 turn=37/40
link p1 arm2 normal
link arm2 p2 normal
link p2 arm4 normal
link arm4 p3 normal
link p3 arm6 normal
link arm6 p4 normal
link p4 arm8 normal
link arm8 middle strong
)tpl"
     R"tpl(
name Spiral
players 4
sizes large-huge

zone middle treasure rich town r=0 turn=0
zone p1 start r=0.38 turn=0
zone arm2 treasure r=0.362 turn=4/15
zone arm3 treasure r=0.344 turn=8/15
zone p2 start r=0.325 turn=4/5
zone arm5 treasure r=0.307 turn=1/15
zone arm6 treasure r=0.289 turn=1/3
zone p3 start r=0.271 turn=3/5
zone arm8 treasure r=0.253 turn=13/15
zone arm9 treasure rich r=0.235 turn=2/15
zone p4 start r=0.216 turn=2/5
zone arm11 treasure rich r=0.198 turn=2/3
zone arm12 treasure rich r=0.18 turn=14/15
link p1 arm2 normal
link arm2 arm3 normal
link arm3 p2 normal
link p2 arm5 normal
link arm5 arm6 normal
link arm6 p3 normal
link p3 arm8 normal
link arm8 arm9 normal
link arm9 p4 normal
link p4 arm11 normal
link arm11 arm12 normal
link arm12 middle strong
)tpl"},
    {"star.txt",
     R"tpl(# Star: the starts on a star's points, an inner ring of zones between them (and on large
# maps a rich middle).
)tpl"
     R"tpl(
name Star
players 3
sizes small-medium

zone p1 start size=1.1 r=0.38 turn=0
zone p2 start size=1.1 r=0.38 turn=1/3
zone p3 start size=1.1 r=0.38 turn=2/3
zone inner1 treasure rich r=0.2 turn=1/6
zone inner2 treasure rich r=0.2 turn=1/2
zone inner3 treasure rich r=0.2 turn=5/6
link p1 inner1 normal
link p1 inner3 normal
link inner1 inner2 strong
link p2 inner2 normal
link p2 inner1 normal
link inner2 inner3 strong
link p3 inner3 normal
link p3 inner2 normal
link inner3 inner1 strong
)tpl"
     R"tpl(
name Star
players 3
sizes large-huge

zone p1 start size=1.1 r=0.38 turn=0
zone p2 start size=1.1 r=0.38 turn=1/3
zone p3 start size=1.1 r=0.38 turn=2/3
zone inner1 treasure rich r=0.2 turn=1/6
zone inner2 treasure rich r=0.2 turn=1/2
zone inner3 treasure rich r=0.2 turn=5/6
zone tip1 treasure size=0.8 r=0.38 turn=1/6
zone tip2 treasure size=0.8 r=0.38 turn=1/2
zone tip3 treasure size=0.8 r=0.38 turn=5/6
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 inner1 normal
link p1 inner3 normal
link inner1 inner2 strong
link tip1 inner1 strong
link p2 inner2 normal
link p2 inner1 normal
link inner2 inner3 strong
link tip2 inner2 strong
link p3 inner3 normal
link p3 inner2 normal
link inner3 inner1 strong
link tip3 inner3 strong
link inner1 middle deadly
link inner2 middle deadly
link inner3 middle deadly
)tpl"
     R"tpl(
name Star
players 4
sizes small-medium

zone p1 start size=1.1 r=0.38 turn=0
zone p2 start size=1.1 r=0.38 turn=1/4
zone p3 start size=1.1 r=0.38 turn=1/2
zone p4 start size=1.1 r=0.38 turn=3/4
zone inner1 treasure r=0.2 turn=1/8
zone inner2 treasure rich r=0.2 turn=3/8
zone inner3 treasure r=0.2 turn=5/8
zone inner4 treasure rich r=0.2 turn=7/8
link p1 inner1 normal
link p1 inner4 normal
link inner1 inner2 strong
link p2 inner2 normal
link p2 inner1 normal
link inner2 inner3 strong
link p3 inner3 normal
link p3 inner2 normal
link inner3 inner4 strong
link p4 inner4 normal
link p4 inner3 normal
link inner4 inner1 strong
)tpl"
     R"tpl(
name Star
players 4
sizes large-huge

zone p1 start size=1.1 r=0.38 turn=0
zone p2 start size=1.1 r=0.38 turn=1/4
zone p3 start size=1.1 r=0.38 turn=1/2
zone p4 start size=1.1 r=0.38 turn=3/4
zone inner1 treasure r=0.2 turn=1/8
zone inner2 treasure rich r=0.2 turn=3/8
zone inner3 treasure r=0.2 turn=5/8
zone inner4 treasure rich r=0.2 turn=7/8
zone tip1 treasure size=0.8 r=0.38 turn=1/8
zone tip2 treasure size=0.8 r=0.38 turn=3/8
zone tip3 treasure size=0.8 r=0.38 turn=5/8
zone tip4 treasure size=0.8 r=0.38 turn=7/8
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 inner1 normal
link p1 inner4 normal
link inner1 inner2 strong
link tip1 inner1 strong
link p2 inner2 normal
link p2 inner1 normal
link inner2 inner3 strong
link tip2 inner2 strong
link p3 inner3 normal
link p3 inner2 normal
link inner3 inner4 strong
link tip3 inner3 strong
link p4 inner4 normal
link p4 inner3 normal
link inner4 inner1 strong
link tip4 inner4 strong
link inner1 middle deadly
link inner2 middle deadly
link inner3 middle deadly
link inner4 middle deadly
)tpl"
     R"tpl(
name Star
players 5
sizes small-medium

zone p1 start size=1.1 r=0.38 turn=0
zone p2 start size=1.1 r=0.38 turn=1/5
zone p3 start size=1.1 r=0.38 turn=2/5
zone p4 start size=1.1 r=0.38 turn=3/5
zone p5 start size=1.1 r=0.38 turn=4/5
zone inner1 treasure r=0.2 turn=1/10
zone inner2 treasure rich r=0.2 turn=3/10
zone inner3 treasure r=0.2 turn=1/2
zone inner4 treasure rich r=0.2 turn=7/10
zone inner5 treasure r=0.2 turn=9/10
link p1 inner1 normal
link p1 inner5 normal
link inner1 inner2 strong
link p2 inner2 normal
link p2 inner1 normal
link inner2 inner3 strong
link p3 inner3 normal
link p3 inner2 normal
link inner3 inner4 strong
link p4 inner4 normal
link p4 inner3 normal
link inner4 inner5 strong
link p5 inner5 normal
link p5 inner4 normal
link inner5 inner1 strong
)tpl"
     R"tpl(
name Star
players 5
sizes large-huge

zone p1 start size=1.1 r=0.38 turn=0
zone p2 start size=1.1 r=0.38 turn=1/5
zone p3 start size=1.1 r=0.38 turn=2/5
zone p4 start size=1.1 r=0.38 turn=3/5
zone p5 start size=1.1 r=0.38 turn=4/5
zone inner1 treasure r=0.2 turn=1/10
zone inner2 treasure rich r=0.2 turn=3/10
zone inner3 treasure r=0.2 turn=1/2
zone inner4 treasure rich r=0.2 turn=7/10
zone inner5 treasure r=0.2 turn=9/10
zone tip1 treasure size=0.8 r=0.38 turn=1/10
zone tip2 treasure size=0.8 r=0.38 turn=3/10
zone tip3 treasure size=0.8 r=0.38 turn=1/2
zone tip4 treasure size=0.8 r=0.38 turn=7/10
zone tip5 treasure size=0.8 r=0.38 turn=9/10
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 inner1 normal
link p1 inner5 normal
link inner1 inner2 strong
link tip1 inner1 strong
link p2 inner2 normal
link p2 inner1 normal
link inner2 inner3 strong
link tip2 inner2 strong
link p3 inner3 normal
link p3 inner2 normal
link inner3 inner4 strong
link tip3 inner3 strong
link p4 inner4 normal
link p4 inner3 normal
link inner4 inner5 strong
link tip4 inner4 strong
link p5 inner5 normal
link p5 inner4 normal
link inner5 inner1 strong
link tip5 inner5 strong
link inner1 middle deadly
link inner2 middle deadly
link inner3 middle deadly
link inner4 middle deadly
link inner5 middle deadly
)tpl"
     R"tpl(
name Star
players 6
sizes small-medium

zone p1 start size=1.1 r=0.38 turn=0
zone p2 start size=1.1 r=0.38 turn=1/6
zone p3 start size=1.1 r=0.38 turn=1/3
zone p4 start size=1.1 r=0.38 turn=1/2
zone p5 start size=1.1 r=0.38 turn=2/3
zone p6 start size=1.1 r=0.38 turn=5/6
zone inner1 treasure r=0.2 turn=1/12
zone inner2 treasure rich r=0.2 turn=1/4
zone inner3 treasure r=0.2 turn=5/12
zone inner4 treasure rich r=0.2 turn=7/12
zone inner5 treasure r=0.2 turn=3/4
zone inner6 treasure rich r=0.2 turn=11/12
link p1 inner1 normal
link p1 inner6 normal
link inner1 inner2 strong
link p2 inner2 normal
link p2 inner1 normal
link inner2 inner3 strong
link p3 inner3 normal
link p3 inner2 normal
link inner3 inner4 strong
link p4 inner4 normal
link p4 inner3 normal
link inner4 inner5 strong
link p5 inner5 normal
link p5 inner4 normal
link inner5 inner6 strong
link p6 inner6 normal
link p6 inner5 normal
link inner6 inner1 strong
)tpl"
     R"tpl(
name Star
players 6
sizes large-huge

zone p1 start size=1.1 r=0.38 turn=0
zone p2 start size=1.1 r=0.38 turn=1/6
zone p3 start size=1.1 r=0.38 turn=1/3
zone p4 start size=1.1 r=0.38 turn=1/2
zone p5 start size=1.1 r=0.38 turn=2/3
zone p6 start size=1.1 r=0.38 turn=5/6
zone inner1 treasure r=0.2 turn=1/12
zone inner2 treasure rich r=0.2 turn=1/4
zone inner3 treasure r=0.2 turn=5/12
zone inner4 treasure rich r=0.2 turn=7/12
zone inner5 treasure r=0.2 turn=3/4
zone inner6 treasure rich r=0.2 turn=11/12
zone tip1 treasure size=0.8 r=0.38 turn=1/12
zone tip2 treasure size=0.8 r=0.38 turn=1/4
zone tip3 treasure size=0.8 r=0.38 turn=5/12
zone tip4 treasure size=0.8 r=0.38 turn=7/12
zone tip5 treasure size=0.8 r=0.38 turn=3/4
zone tip6 treasure size=0.8 r=0.38 turn=11/12
zone middle treasure rich town size=0.8 r=0 turn=0
link p1 inner1 normal
link p1 inner6 normal
link inner1 inner2 strong
link tip1 inner1 strong
link p2 inner2 normal
link p2 inner1 normal
link inner2 inner3 strong
link tip2 inner2 strong
link p3 inner3 normal
link p3 inner2 normal
link inner3 inner4 strong
link tip3 inner3 strong
link p4 inner4 normal
link p4 inner3 normal
link inner4 inner5 strong
link tip4 inner4 strong
link p5 inner5 normal
link p5 inner4 normal
link inner5 inner6 strong
link tip5 inner5 strong
link p6 inner6 normal
link p6 inner5 normal
link inner6 inner1 strong
link tip6 inner6 strong
link inner1 middle deadly
link inner2 middle deadly
link inner3 middle deadly
link inner4 middle deadly
link inner5 middle deadly
link inner6 middle deadly
)tpl"},
    {"three_on_three.txt",
     R"tpl(# Three on three: six players, two teams (players 1, 3, 5 and 2, 4, 6: the teams line), each
# team's starts in a row on its side, the fronts between.
)tpl"
     R"tpl(
name Three on three
players 6
teams 1,3,5 2,4,6
sizes small-medium

zone p1 start x=-0.34 y=-0.3
zone p2 start x=0.34 y=0.3
zone p3 start x=-0.34 y=0
zone p4 start x=0.34 y=0
zone p5 start x=-0.34 y=0.3
zone p6 start x=0.34 y=-0.3
zone west treasure size=0.6 x=-0.36 y=0.15
zone east treasure size=0.6 x=0.36 y=-0.15
zone front1 treasure x=0 y=-0.3
zone front2 treasure rich town x=0 y=0
zone front3 treasure x=0 y=0.3
link p3 west weak
link p5 west weak
link p4 east weak
link p6 east weak
link p1 front1 strong
link p6 front1 strong
link p3 front2 strong
link p4 front2 strong
link p5 front3 strong
link p2 front3 strong
link front1 front2 normal
link front2 front3 normal
link p1 p3 weak
link p6 p4 weak
)tpl"
     R"tpl(
name Three on three
players 6
teams 1,3,5 2,4,6
sizes large-huge

zone p1 start x=-0.34 y=-0.3
zone p2 start x=0.34 y=0.3
zone p3 start x=-0.34 y=0
zone p4 start x=0.34 y=0
zone p5 start x=-0.34 y=0.3
zone p6 start x=0.34 y=-0.3
zone west treasure size=0.6 x=-0.36 y=0.15
zone east treasure size=0.6 x=0.36 y=-0.15
zone front1 treasure x=0 y=-0.3
zone front2 treasure rich town x=0 y=0
zone front3 treasure x=0 y=0.3
link p3 west weak
link p5 west weak
link p4 east weak
link p6 east weak
link p1 front1 strong
link p6 front1 strong
link p3 front2 strong
link p4 front2 strong
link p5 front3 strong
link p2 front3 strong
link front1 front2 normal
link front2 front3 normal
link p1 p3 weak
link p6 p4 weak
)tpl"},
    {"treasury.txt",
     R"tpl(# Treasury: small rich vaults round the middle, each deadly guarded.
)tpl"
     R"tpl(
name Treasury
players 2
sizes small-medium

zone middle treasure town r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/2
zone vault1 treasure rich size=0.5 r=0.27 turn=1/4
zone vault2 treasure rich size=0.5 r=0.27 turn=3/4
link p1 middle normal
link p2 middle normal
link middle vault1 deadly
link middle vault2 deadly
)tpl"
     R"tpl(
name Treasury
players 2
sizes large-huge

zone middle treasure town r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/2
zone vault1 treasure rich size=0.5 r=0.27 turn=1/8
zone vault2 treasure rich size=0.5 r=0.27 turn=3/8
zone vault3 treasure rich size=0.5 r=0.27 turn=5/8
zone vault4 treasure rich size=0.5 r=0.27 turn=7/8
link p1 middle normal
link p2 middle normal
link middle vault1 deadly
link p1 vault1 strong
link middle vault2 deadly
link p1 vault2 strong
link middle vault3 deadly
link p2 vault3 strong
link middle vault4 deadly
link p2 vault4 strong
)tpl"
     R"tpl(
name Treasury
players 3
sizes small-medium

zone middle treasure town r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/3
zone p3 start r=0.37 turn=2/3
zone vault1 treasure rich size=0.5 r=0.27 turn=1/6
zone vault2 treasure rich size=0.5 r=0.27 turn=1/2
zone vault3 treasure rich size=0.5 r=0.27 turn=5/6
link p1 middle normal
link p2 middle normal
link p3 middle normal
link middle vault1 deadly
link middle vault2 deadly
link middle vault3 deadly
)tpl"
     R"tpl(
name Treasury
players 3
sizes large-huge

zone middle treasure town r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/3
zone p3 start r=0.37 turn=2/3
zone vault1 treasure rich size=0.5 r=0.27 turn=1/12
zone vault2 treasure rich size=0.5 r=0.27 turn=1/4
zone vault3 treasure rich size=0.5 r=0.27 turn=5/12
zone vault4 treasure rich size=0.5 r=0.27 turn=7/12
zone vault5 treasure rich size=0.5 r=0.27 turn=3/4
zone vault6 treasure rich size=0.5 r=0.27 turn=11/12
link p1 middle normal
link p2 middle normal
link p3 middle normal
link middle vault1 deadly
link p1 vault1 strong
link middle vault2 deadly
link p1 vault2 strong
link middle vault3 deadly
link p2 vault3 strong
link middle vault4 deadly
link p2 vault4 strong
link middle vault5 deadly
link p3 vault5 strong
link middle vault6 deadly
link p3 vault6 strong
)tpl"
     R"tpl(
name Treasury
players 4
sizes small-medium

zone middle treasure town r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/4
zone p3 start r=0.37 turn=1/2
zone p4 start r=0.37 turn=3/4
zone vault1 treasure rich size=0.5 r=0.27 turn=1/8
zone vault2 treasure rich size=0.5 r=0.27 turn=3/8
zone vault3 treasure rich size=0.5 r=0.27 turn=5/8
zone vault4 treasure rich size=0.5 r=0.27 turn=7/8
link p1 middle normal
link p2 middle normal
link p3 middle normal
link p4 middle normal
link middle vault1 deadly
link middle vault2 deadly
link middle vault3 deadly
link middle vault4 deadly
)tpl"
     R"tpl(
name Treasury
players 4
sizes large-huge

zone middle treasure town r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/4
zone p3 start r=0.37 turn=1/2
zone p4 start r=0.37 turn=3/4
zone vault1 treasure rich size=0.5 r=0.27 turn=1/16
zone vault2 treasure rich size=0.5 r=0.27 turn=3/16
zone vault3 treasure rich size=0.5 r=0.27 turn=5/16
zone vault4 treasure rich size=0.5 r=0.27 turn=7/16
zone vault5 treasure rich size=0.5 r=0.27 turn=9/16
zone vault6 treasure rich size=0.5 r=0.27 turn=11/16
zone vault7 treasure rich size=0.5 r=0.27 turn=13/16
zone vault8 treasure rich size=0.5 r=0.27 turn=15/16
link p1 middle normal
link p2 middle normal
link p3 middle normal
link p4 middle normal
link middle vault1 deadly
link p1 vault1 strong
link middle vault2 deadly
link p1 vault2 strong
link middle vault3 deadly
link p2 vault3 strong
link middle vault4 deadly
link p2 vault4 strong
link middle vault5 deadly
link p3 vault5 strong
link middle vault6 deadly
link p3 vault6 strong
link middle vault7 deadly
link p4 vault7 strong
link middle vault8 deadly
link p4 vault8 strong
)tpl"
     R"tpl(
name Treasury
players 5
sizes small-medium

zone middle treasure town r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/5
zone p3 start r=0.37 turn=2/5
zone p4 start r=0.37 turn=3/5
zone p5 start r=0.37 turn=4/5
zone vault1 treasure rich size=0.5 r=0.27 turn=1/10
zone vault2 treasure rich size=0.5 r=0.27 turn=3/10
zone vault3 treasure rich size=0.5 r=0.27 turn=1/2
zone vault4 treasure rich size=0.5 r=0.27 turn=7/10
zone vault5 treasure rich size=0.5 r=0.27 turn=9/10
link p1 middle normal
link p2 middle normal
link p3 middle normal
link p4 middle normal
link p5 middle normal
link middle vault1 deadly
link middle vault2 deadly
link middle vault3 deadly
link middle vault4 deadly
link middle vault5 deadly
)tpl"
     R"tpl(
name Treasury
players 5
sizes large-huge

zone middle treasure town r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/5
zone p3 start r=0.37 turn=2/5
zone p4 start r=0.37 turn=3/5
zone p5 start r=0.37 turn=4/5
zone vault1 treasure rich size=0.5 r=0.27 turn=1/20
zone vault2 treasure rich size=0.5 r=0.27 turn=3/20
zone vault3 treasure rich size=0.5 r=0.27 turn=1/4
zone vault4 treasure rich size=0.5 r=0.27 turn=7/20
zone vault5 treasure rich size=0.5 r=0.27 turn=9/20
zone vault6 treasure rich size=0.5 r=0.27 turn=11/20
zone vault7 treasure rich size=0.5 r=0.27 turn=13/20
zone vault8 treasure rich size=0.5 r=0.27 turn=3/4
zone vault9 treasure rich size=0.5 r=0.27 turn=17/20
zone vault10 treasure rich size=0.5 r=0.27 turn=19/20
link p1 middle normal
link p2 middle normal
link p3 middle normal
link p4 middle normal
link p5 middle normal
link middle vault1 deadly
link p1 vault1 strong
link middle vault2 deadly
link p1 vault2 strong
link middle vault3 deadly
link p2 vault3 strong
link middle vault4 deadly
link p2 vault4 strong
link middle vault5 deadly
link p3 vault5 strong
link middle vault6 deadly
link p3 vault6 strong
link middle vault7 deadly
link p4 vault7 strong
link middle vault8 deadly
link p4 vault8 strong
link middle vault9 deadly
link p5 vault9 strong
link middle vault10 deadly
link p5 vault10 strong
)tpl"
     R"tpl(
name Treasury
players 6
sizes small-medium

zone middle treasure town r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/6
zone p3 start r=0.37 turn=1/3
zone p4 start r=0.37 turn=1/2
zone p5 start r=0.37 turn=2/3
zone p6 start r=0.37 turn=5/6
zone vault1 treasure rich size=0.5 r=0.27 turn=1/12
zone vault2 treasure rich size=0.5 r=0.27 turn=1/4
zone vault3 treasure rich size=0.5 r=0.27 turn=5/12
zone vault4 treasure rich size=0.5 r=0.27 turn=7/12
zone vault5 treasure rich size=0.5 r=0.27 turn=3/4
zone vault6 treasure rich size=0.5 r=0.27 turn=11/12
link p1 middle normal
link p2 middle normal
link p3 middle normal
link p4 middle normal
link p5 middle normal
link p6 middle normal
link middle vault1 deadly
link middle vault2 deadly
link middle vault3 deadly
link middle vault4 deadly
link middle vault5 deadly
link middle vault6 deadly
)tpl"
     R"tpl(
name Treasury
players 6
sizes large-huge

zone middle treasure town r=0 turn=0
zone p1 start r=0.37 turn=0
zone p2 start r=0.37 turn=1/6
zone p3 start r=0.37 turn=1/3
zone p4 start r=0.37 turn=1/2
zone p5 start r=0.37 turn=2/3
zone p6 start r=0.37 turn=5/6
zone vault1 treasure rich size=0.5 r=0.27 turn=1/24
zone vault2 treasure rich size=0.5 r=0.27 turn=1/8
zone vault3 treasure rich size=0.5 r=0.27 turn=5/24
zone vault4 treasure rich size=0.5 r=0.27 turn=7/24
zone vault5 treasure rich size=0.5 r=0.27 turn=3/8
zone vault6 treasure rich size=0.5 r=0.27 turn=11/24
zone vault7 treasure rich size=0.5 r=0.27 turn=13/24
zone vault8 treasure rich size=0.5 r=0.27 turn=5/8
zone vault9 treasure rich size=0.5 r=0.27 turn=17/24
zone vault10 treasure rich size=0.5 r=0.27 turn=19/24
zone vault11 treasure rich size=0.5 r=0.27 turn=7/8
zone vault12 treasure rich size=0.5 r=0.27 turn=23/24
link p1 middle normal
link p2 middle normal
link p3 middle normal
link p4 middle normal
link p5 middle normal
link p6 middle normal
link middle vault1 deadly
link p1 vault1 strong
link middle vault2 deadly
link p1 vault2 strong
link middle vault3 deadly
link p2 vault3 strong
link middle vault4 deadly
link p2 vault4 strong
link middle vault5 deadly
link p3 vault5 strong
link middle vault6 deadly
link p3 vault6 strong
link middle vault7 deadly
link p4 vault7 strong
link middle vault8 deadly
link p4 vault8 strong
link middle vault9 deadly
link p5 vault9 strong
link middle vault10 deadly
link p5 vault10 strong
link middle vault11 deadly
link p6 vault11 strong
link middle vault12 deadly
link p6 vault12 strong
)tpl"},
    {"triangle.txt",
     R"tpl(# Triangle: three players, a zone between each two, a rich middle with a town.
)tpl"
     R"tpl(
name Triangle
players 3
sizes small-medium

zone middle treasure rich town size=1.3 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=0
zone p2 start size=1.2 r=0.37 turn=1/3
zone p3 start size=1.2 r=0.37 turn=2/3
zone side1 treasure r=0.33 turn=1/6
zone side2 treasure r=0.33 turn=1/2
zone side3 treasure r=0.33 turn=5/6
link p1 side1 normal
link p2 side1 normal
link side1 middle strong
link p2 side2 normal
link p3 side2 normal
link side2 middle strong
link p3 side3 normal
link p1 side3 normal
link side3 middle strong
)tpl"
     R"tpl(
name Triangle
players 3
sizes large-huge

zone middle treasure rich town size=1.3 r=0 turn=0
zone p1 start size=1.2 r=0.37 turn=0
zone p2 start size=1.2 r=0.37 turn=1/3
zone p3 start size=1.2 r=0.37 turn=2/3
zone side1 treasure r=0.33 turn=1/6
zone inner1 treasure r=0.18 turn=0
zone side2 treasure r=0.33 turn=1/2
zone inner2 treasure r=0.18 turn=1/3
zone side3 treasure r=0.33 turn=5/6
zone inner3 treasure r=0.18 turn=2/3
link p1 side1 normal
link p2 side1 normal
link side1 middle strong
link p1 inner1 weak
link inner1 middle strong
link p2 side2 normal
link p3 side2 normal
link side2 middle strong
link p2 inner2 weak
link inner2 middle strong
link p3 side3 normal
link p1 side3 normal
link side3 middle strong
link p3 inner3 weak
link inner3 middle strong
)tpl"},
    {"twin_rings.txt",
     R"tpl(# Twin rings: two players, each in a ring of own zones; the rings touch at two rich gates.
)tpl"
     R"tpl(
name Twin rings
players 2
sizes small-medium

zone p1 start size=1.2 x=-0.34 y=0
zone north1 treasure x=-0.22 y=-0.26
zone south1 treasure x=-0.22 y=0.26
zone p2 start size=1.2 x=0.34 y=0
zone north2 treasure x=0.22 y=-0.26
zone south2 treasure x=0.22 y=0.26
zone gate_north treasure rich size=0.8 x=0 y=-0.3
zone gate_south treasure rich town size=0.8 x=0 y=0.3
link p1 north1 weak
link p1 south1 weak
link p2 north2 weak
link p2 south2 weak
link north1 gate_north strong
link south1 gate_south strong
link north2 gate_north strong
link south2 gate_south strong
)tpl"
     R"tpl(
name Twin rings
players 2
sizes large-huge

zone p1 start size=1.2 x=-0.34 y=0
zone north1 treasure x=-0.22 y=-0.26
zone south1 treasure x=-0.22 y=0.26
zone inner1 treasure rich size=0.8 x=-0.16 y=0
zone p2 start size=1.2 x=0.34 y=0
zone north2 treasure x=0.22 y=-0.26
zone south2 treasure x=0.22 y=0.26
zone inner2 treasure rich size=0.8 x=0.16 y=0
zone gate_north treasure rich size=0.8 x=0 y=-0.3
zone gate_south treasure rich town size=0.8 x=0 y=0.3
link p1 north1 weak
link p1 south1 weak
link north1 inner1 normal
link south1 inner1 normal
link p2 north2 weak
link p2 south2 weak
link north2 inner2 normal
link south2 inner2 normal
link north1 gate_north strong
link south1 gate_south strong
link north2 gate_north strong
link south2 gate_south strong
)tpl"},
    {"web.txt",
     R"tpl(# Web: two rings of small zones and many ways between them, a rich middle.
)tpl"
     R"tpl(
name Web
players 2
sizes small-medium

zone p1 start size=1.1 r=0.37 turn=0
zone p2 start size=1.1 r=0.37 turn=1/2
zone outer1 treasure size=0.8 r=0.37 turn=1/4
zone inner1 treasure size=0.8 r=0.2 turn=0
zone outer2 treasure size=0.8 r=0.37 turn=3/4
zone inner2 treasure rich size=0.8 r=0.2 turn=1/2
zone middle treasure rich town size=0.9 r=0 turn=0
link p1 outer1 weak
link outer1 p2 weak
link p1 inner1 normal
link inner1 inner2 normal
link outer1 inner1 strong
link p2 outer2 weak
link outer2 p1 weak
link p2 inner2 normal
link inner2 inner1 normal
link outer2 inner2 strong
link inner1 middle strong
link inner2 middle strong
)tpl"
     R"tpl(
name Web
players 2
sizes large-huge

zone p1 start size=1.1 r=0.37 turn=0
zone p2 start size=1.1 r=0.37 turn=1/2
zone outer1 treasure size=0.8 r=0.37 turn=1/4
zone inner1 treasure size=0.8 r=0.2 turn=0
zone outer2 treasure size=0.8 r=0.37 turn=3/4
zone inner2 treasure rich size=0.8 r=0.2 turn=1/2
zone middle treasure rich town size=1.2 r=0 turn=0
link p1 outer1 weak
link outer1 p2 weak
link p1 inner1 normal
link inner1 inner2 normal
link outer1 inner1 strong
link p2 outer2 weak
link outer2 p1 weak
link p2 inner2 normal
link inner2 inner1 normal
link outer2 inner2 strong
link inner1 middle strong
link inner2 middle strong
)tpl"
     R"tpl(
name Web
players 3
sizes small-medium

zone p1 start size=1.1 r=0.37 turn=0
zone p2 start size=1.1 r=0.37 turn=1/3
zone p3 start size=1.1 r=0.37 turn=2/3
zone outer1 treasure size=0.8 r=0.37 turn=1/6
zone inner1 treasure size=0.8 r=0.2 turn=0
zone outer2 treasure size=0.8 r=0.37 turn=1/2
zone inner2 treasure rich size=0.8 r=0.2 turn=1/3
zone outer3 treasure size=0.8 r=0.37 turn=5/6
zone inner3 treasure size=0.8 r=0.2 turn=2/3
zone middle treasure rich town size=0.9 r=0 turn=0
link p1 outer1 weak
link outer1 p2 weak
link p1 inner1 normal
link inner1 inner2 normal
link outer1 inner1 strong
link p2 outer2 weak
link outer2 p3 weak
link p2 inner2 normal
link inner2 inner3 normal
link outer2 inner2 strong
link p3 outer3 weak
link outer3 p1 weak
link p3 inner3 normal
link inner3 inner1 normal
link outer3 inner3 strong
link inner1 middle strong
link inner2 middle strong
link inner3 middle strong
)tpl"
     R"tpl(
name Web
players 3
sizes large-huge

zone p1 start size=1.1 r=0.37 turn=0
zone p2 start size=1.1 r=0.37 turn=1/3
zone p3 start size=1.1 r=0.37 turn=2/3
zone outer1 treasure size=0.8 r=0.37 turn=1/6
zone inner1 treasure size=0.8 r=0.2 turn=0
zone outer2 treasure size=0.8 r=0.37 turn=1/2
zone inner2 treasure rich size=0.8 r=0.2 turn=1/3
zone outer3 treasure size=0.8 r=0.37 turn=5/6
zone inner3 treasure size=0.8 r=0.2 turn=2/3
zone middle treasure rich town size=1.2 r=0 turn=0
link p1 outer1 weak
link outer1 p2 weak
link p1 inner1 normal
link inner1 inner2 normal
link outer1 inner1 strong
link p2 outer2 weak
link outer2 p3 weak
link p2 inner2 normal
link inner2 inner3 normal
link outer2 inner2 strong
link p3 outer3 weak
link outer3 p1 weak
link p3 inner3 normal
link inner3 inner1 normal
link outer3 inner3 strong
link inner1 middle strong
link inner2 middle strong
link inner3 middle strong
)tpl"
     R"tpl(
name Web
players 4
sizes small-medium

zone p1 start size=1.1 r=0.37 turn=0
zone p2 start size=1.1 r=0.37 turn=1/4
zone p3 start size=1.1 r=0.37 turn=1/2
zone p4 start size=1.1 r=0.37 turn=3/4
zone outer1 treasure size=0.8 r=0.37 turn=1/8
zone inner1 treasure size=0.8 r=0.2 turn=0
zone outer2 treasure size=0.8 r=0.37 turn=3/8
zone inner2 treasure rich size=0.8 r=0.2 turn=1/4
zone outer3 treasure size=0.8 r=0.37 turn=5/8
zone inner3 treasure size=0.8 r=0.2 turn=1/2
zone outer4 treasure size=0.8 r=0.37 turn=7/8
zone inner4 treasure rich size=0.8 r=0.2 turn=3/4
zone middle treasure rich town size=0.9 r=0 turn=0
link p1 outer1 weak
link outer1 p2 weak
link p1 inner1 normal
link inner1 inner2 normal
link outer1 inner1 strong
link p2 outer2 weak
link outer2 p3 weak
link p2 inner2 normal
link inner2 inner3 normal
link outer2 inner2 strong
link p3 outer3 weak
link outer3 p4 weak
link p3 inner3 normal
link inner3 inner4 normal
link outer3 inner3 strong
link p4 outer4 weak
link outer4 p1 weak
link p4 inner4 normal
link inner4 inner1 normal
link outer4 inner4 strong
link inner1 middle strong
link inner2 middle strong
link inner3 middle strong
link inner4 middle strong
)tpl"
     R"tpl(
name Web
players 4
sizes large-huge

zone p1 start size=1.1 r=0.37 turn=0
zone p2 start size=1.1 r=0.37 turn=1/4
zone p3 start size=1.1 r=0.37 turn=1/2
zone p4 start size=1.1 r=0.37 turn=3/4
zone outer1 treasure size=0.8 r=0.37 turn=1/8
zone inner1 treasure size=0.8 r=0.2 turn=0
zone outer2 treasure size=0.8 r=0.37 turn=3/8
zone inner2 treasure rich size=0.8 r=0.2 turn=1/4
zone outer3 treasure size=0.8 r=0.37 turn=5/8
zone inner3 treasure size=0.8 r=0.2 turn=1/2
zone outer4 treasure size=0.8 r=0.37 turn=7/8
zone inner4 treasure rich size=0.8 r=0.2 turn=3/4
zone middle treasure rich town size=1.2 r=0 turn=0
link p1 outer1 weak
link outer1 p2 weak
link p1 inner1 normal
link inner1 inner2 normal
link outer1 inner1 strong
link p2 outer2 weak
link outer2 p3 weak
link p2 inner2 normal
link inner2 inner3 normal
link outer2 inner2 strong
link p3 outer3 weak
link outer3 p4 weak
link p3 inner3 normal
link inner3 inner4 normal
link outer3 inner3 strong
link p4 outer4 weak
link outer4 p1 weak
link p4 inner4 normal
link inner4 inner1 normal
link outer4 inner4 strong
link inner1 middle strong
link inner2 middle strong
link inner3 middle strong
link inner4 middle strong
)tpl"
     R"tpl(
name Web
players 5
sizes medium

zone p1 start size=1.1 r=0.37 turn=0
zone p2 start size=1.1 r=0.37 turn=1/5
zone p3 start size=1.1 r=0.37 turn=2/5
zone p4 start size=1.1 r=0.37 turn=3/5
zone p5 start size=1.1 r=0.37 turn=4/5
zone outer1 treasure size=0.8 r=0.37 turn=1/10
zone inner1 treasure size=0.8 r=0.2 turn=0
zone outer2 treasure size=0.8 r=0.37 turn=3/10
zone inner2 treasure rich size=0.8 r=0.2 turn=1/5
zone outer3 treasure size=0.8 r=0.37 turn=1/2
zone inner3 treasure size=0.8 r=0.2 turn=2/5
zone outer4 treasure size=0.8 r=0.37 turn=7/10
zone inner4 treasure rich size=0.8 r=0.2 turn=3/5
zone outer5 treasure size=0.8 r=0.37 turn=9/10
zone inner5 treasure size=0.8 r=0.2 turn=4/5
zone middle treasure rich town size=0.9 r=0 turn=0
link p1 outer1 weak
link outer1 p2 weak
link p1 inner1 normal
link inner1 inner2 normal
link outer1 inner1 strong
link p2 outer2 weak
link outer2 p3 weak
link p2 inner2 normal
link inner2 inner3 normal
link outer2 inner2 strong
link p3 outer3 weak
link outer3 p4 weak
link p3 inner3 normal
link inner3 inner4 normal
link outer3 inner3 strong
link p4 outer4 weak
link outer4 p5 weak
link p4 inner4 normal
link inner4 inner5 normal
link outer4 inner4 strong
link p5 outer5 weak
link outer5 p1 weak
link p5 inner5 normal
link inner5 inner1 normal
link outer5 inner5 strong
link inner1 middle strong
link inner2 middle strong
link inner3 middle strong
link inner4 middle strong
link inner5 middle strong
)tpl"
     R"tpl(
name Web
players 5
sizes large-huge

zone p1 start size=1.1 r=0.37 turn=0
zone p2 start size=1.1 r=0.37 turn=1/5
zone p3 start size=1.1 r=0.37 turn=2/5
zone p4 start size=1.1 r=0.37 turn=3/5
zone p5 start size=1.1 r=0.37 turn=4/5
zone outer1 treasure size=0.8 r=0.37 turn=1/10
zone inner1 treasure size=0.8 r=0.2 turn=0
zone outer2 treasure size=0.8 r=0.37 turn=3/10
zone inner2 treasure rich size=0.8 r=0.2 turn=1/5
zone outer3 treasure size=0.8 r=0.37 turn=1/2
zone inner3 treasure size=0.8 r=0.2 turn=2/5
zone outer4 treasure size=0.8 r=0.37 turn=7/10
zone inner4 treasure rich size=0.8 r=0.2 turn=3/5
zone outer5 treasure size=0.8 r=0.37 turn=9/10
zone inner5 treasure size=0.8 r=0.2 turn=4/5
zone middle treasure rich town size=1.2 r=0 turn=0
link p1 outer1 weak
link outer1 p2 weak
link p1 inner1 normal
link inner1 inner2 normal
link outer1 inner1 strong
link p2 outer2 weak
link outer2 p3 weak
link p2 inner2 normal
link inner2 inner3 normal
link outer2 inner2 strong
link p3 outer3 weak
link outer3 p4 weak
link p3 inner3 normal
link inner3 inner4 normal
link outer3 inner3 strong
link p4 outer4 weak
link outer4 p5 weak
link p4 inner4 normal
link inner4 inner5 normal
link outer4 inner4 strong
link p5 outer5 weak
link outer5 p1 weak
link p5 inner5 normal
link inner5 inner1 normal
link outer5 inner5 strong
link inner1 middle strong
link inner2 middle strong
link inner3 middle strong
link inner4 middle strong
link inner5 middle strong
)tpl"
     R"tpl(
name Web
players 6
sizes medium

zone p1 start size=1.1 r=0.37 turn=0
zone p2 start size=1.1 r=0.37 turn=1/6
zone p3 start size=1.1 r=0.37 turn=1/3
zone p4 start size=1.1 r=0.37 turn=1/2
zone p5 start size=1.1 r=0.37 turn=2/3
zone p6 start size=1.1 r=0.37 turn=5/6
zone outer1 treasure size=0.8 r=0.37 turn=1/12
zone inner1 treasure size=0.8 r=0.2 turn=0
zone outer2 treasure size=0.8 r=0.37 turn=1/4
zone inner2 treasure rich size=0.8 r=0.2 turn=1/6
zone outer3 treasure size=0.8 r=0.37 turn=5/12
zone inner3 treasure size=0.8 r=0.2 turn=1/3
zone outer4 treasure size=0.8 r=0.37 turn=7/12
zone inner4 treasure rich size=0.8 r=0.2 turn=1/2
zone outer5 treasure size=0.8 r=0.37 turn=3/4
zone inner5 treasure size=0.8 r=0.2 turn=2/3
zone outer6 treasure size=0.8 r=0.37 turn=11/12
zone inner6 treasure rich size=0.8 r=0.2 turn=5/6
zone middle treasure rich town size=0.9 r=0 turn=0
link p1 outer1 weak
link outer1 p2 weak
link p1 inner1 normal
link inner1 inner2 normal
link outer1 inner1 strong
link p2 outer2 weak
link outer2 p3 weak
link p2 inner2 normal
link inner2 inner3 normal
link outer2 inner2 strong
link p3 outer3 weak
link outer3 p4 weak
link p3 inner3 normal
link inner3 inner4 normal
link outer3 inner3 strong
link p4 outer4 weak
link outer4 p5 weak
link p4 inner4 normal
link inner4 inner5 normal
link outer4 inner4 strong
link p5 outer5 weak
link outer5 p6 weak
link p5 inner5 normal
link inner5 inner6 normal
link outer5 inner5 strong
link p6 outer6 weak
link outer6 p1 weak
link p6 inner6 normal
link inner6 inner1 normal
link outer6 inner6 strong
link inner1 middle strong
link inner2 middle strong
link inner3 middle strong
link inner4 middle strong
link inner5 middle strong
link inner6 middle strong
)tpl"
     R"tpl(
name Web
players 6
sizes large-huge

zone p1 start size=1.1 r=0.37 turn=0
zone p2 start size=1.1 r=0.37 turn=1/6
zone p3 start size=1.1 r=0.37 turn=1/3
zone p4 start size=1.1 r=0.37 turn=1/2
zone p5 start size=1.1 r=0.37 turn=2/3
zone p6 start size=1.1 r=0.37 turn=5/6
zone outer1 treasure size=0.8 r=0.37 turn=1/12
zone inner1 treasure size=0.8 r=0.2 turn=0
zone outer2 treasure size=0.8 r=0.37 turn=1/4
zone inner2 treasure rich size=0.8 r=0.2 turn=1/6
zone outer3 treasure size=0.8 r=0.37 turn=5/12
zone inner3 treasure size=0.8 r=0.2 turn=1/3
zone outer4 treasure size=0.8 r=0.37 turn=7/12
zone inner4 treasure rich size=0.8 r=0.2 turn=1/2
zone outer5 treasure size=0.8 r=0.37 turn=3/4
zone inner5 treasure size=0.8 r=0.2 turn=2/3
zone outer6 treasure size=0.8 r=0.37 turn=11/12
zone inner6 treasure rich size=0.8 r=0.2 turn=5/6
zone middle treasure rich town size=1.2 r=0 turn=0
link p1 outer1 weak
link outer1 p2 weak
link p1 inner1 normal
link inner1 inner2 normal
link outer1 inner1 strong
link p2 outer2 weak
link outer2 p3 weak
link p2 inner2 normal
link inner2 inner3 normal
link outer2 inner2 strong
link p3 outer3 weak
link outer3 p4 weak
link p3 inner3 normal
link inner3 inner4 normal
link outer3 inner3 strong
link p4 outer4 weak
link outer4 p5 weak
link p4 inner4 normal
link inner4 inner5 normal
link outer4 inner4 strong
link p5 outer5 weak
link outer5 p6 weak
link p5 inner5 normal
link inner5 inner6 normal
link outer5 inner5 strong
link p6 outer6 weak
link outer6 p1 weak
link p6 inner6 normal
link inner6 inner1 normal
link outer6 inner6 strong
link inner1 middle strong
link inner2 middle strong
link inner3 middle strong
link inner4 middle strong
link inner5 middle strong
link inner6 middle strong
)tpl"},
    {"wheel.txt",
     R"tpl(# Wheel: the starts on the rim, a spoke from each to a rich hub with a town; rim zones
# between the starts.
)tpl"
     R"tpl(
name Wheel
players 2
sizes small-medium

zone hub treasure rich town size=0.9 r=0 turn=0
zone p1 start r=0.37 turn=0
zone spoke1 treasure size=0.8 r=0.19 turn=0
zone rim1 treasure r=0.38 turn=1/4
zone p2 start r=0.37 turn=1/2
zone spoke2 treasure size=0.8 r=0.19 turn=1/2
zone rim2 treasure r=0.38 turn=3/4
link p1 spoke1 weak
link spoke1 hub strong
link p1 rim1 normal
link p2 spoke2 weak
link spoke2 hub strong
link p2 rim2 normal
link rim1 p2 normal
link rim2 p1 normal
)tpl"
     R"tpl(
name Wheel
players 2
sizes large-huge

zone hub treasure rich town size=0.9 r=0 turn=0
zone p1 start r=0.37 turn=0
zone spoke1 treasure size=0.8 r=0.19 turn=0
zone rim1 treasure rich r=0.38 turn=1/4
zone p2 start r=0.37 turn=1/2
zone spoke2 treasure size=0.8 r=0.19 turn=1/2
zone rim2 treasure rich r=0.38 turn=3/4
link p1 spoke1 weak
link spoke1 hub strong
link p1 rim1 normal
link p2 spoke2 weak
link spoke2 hub strong
link p2 rim2 normal
link rim1 p2 normal
link rim2 p1 normal
)tpl"
     R"tpl(
name Wheel
players 3
sizes small-medium

zone hub treasure rich town size=0.9 r=0 turn=0
zone p1 start r=0.37 turn=0
zone spoke1 treasure size=0.8 r=0.19 turn=0
zone rim1 treasure r=0.38 turn=1/6
zone p2 start r=0.37 turn=1/3
zone spoke2 treasure size=0.8 r=0.19 turn=1/3
zone rim2 treasure r=0.38 turn=1/2
zone p3 start r=0.37 turn=2/3
zone spoke3 treasure size=0.8 r=0.19 turn=2/3
zone rim3 treasure r=0.38 turn=5/6
link p1 spoke1 weak
link spoke1 hub strong
link p1 rim1 normal
link p2 spoke2 weak
link spoke2 hub strong
link p2 rim2 normal
link p3 spoke3 weak
link spoke3 hub strong
link p3 rim3 normal
link rim1 p2 normal
link rim2 p3 normal
link rim3 p1 normal
)tpl"
     R"tpl(
name Wheel
players 3
sizes large-huge

zone hub treasure rich town size=0.9 r=0 turn=0
zone p1 start r=0.37 turn=0
zone spoke1 treasure size=0.8 r=0.19 turn=0
zone rim1 treasure rich r=0.38 turn=1/6
zone p2 start r=0.37 turn=1/3
zone spoke2 treasure size=0.8 r=0.19 turn=1/3
zone rim2 treasure rich r=0.38 turn=1/2
zone p3 start r=0.37 turn=2/3
zone spoke3 treasure size=0.8 r=0.19 turn=2/3
zone rim3 treasure rich r=0.38 turn=5/6
link p1 spoke1 weak
link spoke1 hub strong
link p1 rim1 normal
link p2 spoke2 weak
link spoke2 hub strong
link p2 rim2 normal
link p3 spoke3 weak
link spoke3 hub strong
link p3 rim3 normal
link rim1 p2 normal
link rim2 p3 normal
link rim3 p1 normal
)tpl"
     R"tpl(
name Wheel
players 4
sizes small-medium

zone hub treasure rich town size=0.9 r=0 turn=0
zone p1 start r=0.37 turn=0
zone spoke1 treasure size=0.8 r=0.19 turn=0
zone rim1 treasure r=0.38 turn=1/8
zone p2 start r=0.37 turn=1/4
zone spoke2 treasure size=0.8 r=0.19 turn=1/4
zone rim2 treasure r=0.38 turn=3/8
zone p3 start r=0.37 turn=1/2
zone spoke3 treasure size=0.8 r=0.19 turn=1/2
zone rim3 treasure r=0.38 turn=5/8
zone p4 start r=0.37 turn=3/4
zone spoke4 treasure size=0.8 r=0.19 turn=3/4
zone rim4 treasure r=0.38 turn=7/8
link p1 spoke1 weak
link spoke1 hub strong
link p1 rim1 normal
link p2 spoke2 weak
link spoke2 hub strong
link p2 rim2 normal
link p3 spoke3 weak
link spoke3 hub strong
link p3 rim3 normal
link p4 spoke4 weak
link spoke4 hub strong
link p4 rim4 normal
link rim1 p2 normal
link rim2 p3 normal
link rim3 p4 normal
link rim4 p1 normal
)tpl"
     R"tpl(
name Wheel
players 4
sizes large-huge

zone hub treasure rich town size=0.9 r=0 turn=0
zone p1 start r=0.37 turn=0
zone spoke1 treasure size=0.8 r=0.19 turn=0
zone rim1 treasure rich r=0.38 turn=1/8
zone p2 start r=0.37 turn=1/4
zone spoke2 treasure size=0.8 r=0.19 turn=1/4
zone rim2 treasure rich r=0.38 turn=3/8
zone p3 start r=0.37 turn=1/2
zone spoke3 treasure size=0.8 r=0.19 turn=1/2
zone rim3 treasure rich r=0.38 turn=5/8
zone p4 start r=0.37 turn=3/4
zone spoke4 treasure size=0.8 r=0.19 turn=3/4
zone rim4 treasure rich r=0.38 turn=7/8
link p1 spoke1 weak
link spoke1 hub strong
link p1 rim1 normal
link p2 spoke2 weak
link spoke2 hub strong
link p2 rim2 normal
link p3 spoke3 weak
link spoke3 hub strong
link p3 rim3 normal
link p4 spoke4 weak
link spoke4 hub strong
link p4 rim4 normal
link rim1 p2 normal
link rim2 p3 normal
link rim3 p4 normal
link rim4 p1 normal
)tpl"
     R"tpl(
name Wheel
players 5
sizes medium

zone hub treasure rich town size=0.9 r=0 turn=0
zone p1 start r=0.37 turn=0
zone spoke1 treasure size=0.8 r=0.19 turn=0
zone rim1 treasure r=0.38 turn=1/10
zone p2 start r=0.37 turn=1/5
zone spoke2 treasure size=0.8 r=0.19 turn=1/5
zone rim2 treasure r=0.38 turn=3/10
zone p3 start r=0.37 turn=2/5
zone spoke3 treasure size=0.8 r=0.19 turn=2/5
zone rim3 treasure r=0.38 turn=1/2
zone p4 start r=0.37 turn=3/5
zone spoke4 treasure size=0.8 r=0.19 turn=3/5
zone rim4 treasure r=0.38 turn=7/10
zone p5 start r=0.37 turn=4/5
zone spoke5 treasure size=0.8 r=0.19 turn=4/5
zone rim5 treasure r=0.38 turn=9/10
link p1 spoke1 weak
link spoke1 hub strong
link p1 rim1 normal
link p2 spoke2 weak
link spoke2 hub strong
link p2 rim2 normal
link p3 spoke3 weak
link spoke3 hub strong
link p3 rim3 normal
link p4 spoke4 weak
link spoke4 hub strong
link p4 rim4 normal
link p5 spoke5 weak
link spoke5 hub strong
link p5 rim5 normal
link rim1 p2 normal
link rim2 p3 normal
link rim3 p4 normal
link rim4 p5 normal
link rim5 p1 normal
)tpl"
     R"tpl(
name Wheel
players 5
sizes large-huge

zone hub treasure rich town size=0.9 r=0 turn=0
zone p1 start r=0.37 turn=0
zone spoke1 treasure size=0.8 r=0.19 turn=0
zone rim1 treasure rich r=0.38 turn=1/10
zone p2 start r=0.37 turn=1/5
zone spoke2 treasure size=0.8 r=0.19 turn=1/5
zone rim2 treasure rich r=0.38 turn=3/10
zone p3 start r=0.37 turn=2/5
zone spoke3 treasure size=0.8 r=0.19 turn=2/5
zone rim3 treasure rich r=0.38 turn=1/2
zone p4 start r=0.37 turn=3/5
zone spoke4 treasure size=0.8 r=0.19 turn=3/5
zone rim4 treasure rich r=0.38 turn=7/10
zone p5 start r=0.37 turn=4/5
zone spoke5 treasure size=0.8 r=0.19 turn=4/5
zone rim5 treasure rich r=0.38 turn=9/10
link p1 spoke1 weak
link spoke1 hub strong
link p1 rim1 normal
link p2 spoke2 weak
link spoke2 hub strong
link p2 rim2 normal
link p3 spoke3 weak
link spoke3 hub strong
link p3 rim3 normal
link p4 spoke4 weak
link spoke4 hub strong
link p4 rim4 normal
link p5 spoke5 weak
link spoke5 hub strong
link p5 rim5 normal
link rim1 p2 normal
link rim2 p3 normal
link rim3 p4 normal
link rim4 p5 normal
link rim5 p1 normal
)tpl"
     R"tpl(
name Wheel
players 6
sizes medium

zone hub treasure rich town size=0.9 r=0 turn=0
zone p1 start r=0.37 turn=0
zone spoke1 treasure size=0.8 r=0.19 turn=0
zone rim1 treasure r=0.38 turn=1/12
zone p2 start r=0.37 turn=1/6
zone spoke2 treasure size=0.8 r=0.19 turn=1/6
zone rim2 treasure r=0.38 turn=1/4
zone p3 start r=0.37 turn=1/3
zone spoke3 treasure size=0.8 r=0.19 turn=1/3
zone rim3 treasure r=0.38 turn=5/12
zone p4 start r=0.37 turn=1/2
zone spoke4 treasure size=0.8 r=0.19 turn=1/2
zone rim4 treasure r=0.38 turn=7/12
zone p5 start r=0.37 turn=2/3
zone spoke5 treasure size=0.8 r=0.19 turn=2/3
zone rim5 treasure r=0.38 turn=3/4
zone p6 start r=0.37 turn=5/6
zone spoke6 treasure size=0.8 r=0.19 turn=5/6
zone rim6 treasure r=0.38 turn=11/12
link p1 spoke1 weak
link spoke1 hub strong
link p1 rim1 normal
link p2 spoke2 weak
link spoke2 hub strong
link p2 rim2 normal
link p3 spoke3 weak
link spoke3 hub strong
link p3 rim3 normal
link p4 spoke4 weak
link spoke4 hub strong
link p4 rim4 normal
link p5 spoke5 weak
link spoke5 hub strong
link p5 rim5 normal
link p6 spoke6 weak
link spoke6 hub strong
link p6 rim6 normal
link rim1 p2 normal
link rim2 p3 normal
link rim3 p4 normal
link rim4 p5 normal
link rim5 p6 normal
link rim6 p1 normal
)tpl"
     R"tpl(
name Wheel
players 6
sizes large-huge

zone hub treasure rich town size=0.9 r=0 turn=0
zone p1 start r=0.37 turn=0
zone spoke1 treasure size=0.8 r=0.19 turn=0
zone rim1 treasure rich r=0.38 turn=1/12
zone p2 start r=0.37 turn=1/6
zone spoke2 treasure size=0.8 r=0.19 turn=1/6
zone rim2 treasure rich r=0.38 turn=1/4
zone p3 start r=0.37 turn=1/3
zone spoke3 treasure size=0.8 r=0.19 turn=1/3
zone rim3 treasure rich r=0.38 turn=5/12
zone p4 start r=0.37 turn=1/2
zone spoke4 treasure size=0.8 r=0.19 turn=1/2
zone rim4 treasure rich r=0.38 turn=7/12
zone p5 start r=0.37 turn=2/3
zone spoke5 treasure size=0.8 r=0.19 turn=2/3
zone rim5 treasure rich r=0.38 turn=3/4
zone p6 start r=0.37 turn=5/6
zone spoke6 treasure size=0.8 r=0.19 turn=5/6
zone rim6 treasure rich r=0.38 turn=11/12
link p1 spoke1 weak
link spoke1 hub strong
link p1 rim1 normal
link p2 spoke2 weak
link spoke2 hub strong
link p2 rim2 normal
link p3 spoke3 weak
link spoke3 hub strong
link p3 rim3 normal
link p4 spoke4 weak
link spoke4 hub strong
link p4 rim4 normal
link p5 spoke5 weak
link spoke5 hub strong
link p5 rim5 normal
link p6 spoke6 weak
link spoke6 hub strong
link p6 rim6 normal
link rim1 p2 normal
link rim2 p3 normal
link rim3 p4 normal
link rim4 p5 normal
link rim5 p6 normal
link rim6 p1 normal
)tpl"},
};
const int kShippedTemplateCount = 32;

}  // namespace h4::random_map
