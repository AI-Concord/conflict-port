static void init_iran(void) {                    // 1D93:148B
    W(0x92f2) = 4730;
    W(0x9302) = 5000;
    W(0xa040) = 13;
    W(0xa050) = (i16)(rnd(3) + 1);
    W(0xa100) = (i16)(rnd(4) + 1);
    W(0x98c4) = 2;
    W(0x947a) = 1;
    W(0x94ca) = 2;
    W(0x9454) = 2200;
    W(0x9456) = 1500;
    W(0x945c) = 250;
    W(0x9458) = 32;
    W(0x945a) = 100;
    W(0xa110) = 2;
    W(0xa120) = 1;
    W(0xa140) = (i16)(rnd(7) + 6);
    { int t; do t = rnd(8) - 1; while (t == 0 || t == 1 || t == 3 || t == 4 || t == 5 || t == 7); W(0xa150) = (i16)t; }   // initial target
}

static void init_iraq(void) {                    // 1D93:1537
    W(0x92f4) = 1180;
    W(0x9304) = 2500;
    W(0xa042) = 11;
    W(0xa052) = (i16)(rnd(3) + 1);
    W(0xa102) = (i16)(rnd(4) + 1);
    W(0x98c6) = 1;
    W(0x9506) = 1;
    W(0x956a) = 1;
    W(0x952e) = 1;
    W(0x9508) = 1600;
    W(0x950a) = 1400;
    W(0x9510) = 100;
    W(0x950c) = 4;
    W(0x950e) = 100;
    W(0xa112) = 1;
    W(0xa122) = 1;
    W(0xa142) = (i16)(rnd(7) + -1);
    { int t; do t = rnd(8) - 1; while (t == 0 || t == 2 || t == 4 || t == 5 || t == 7); W(0xa152) = (i16)t; }   // initial target
}

static void init_libya(void) {                    // 1D93:15E5
    W(0x92fa) = 1000;
    W(0x930a) = 1000;
    W(0xa048) = 2;
    W(0xa058) = (i16)(rnd(3) + 1);
    W(0xa108) = (i16)(rnd(4) + 2);
    W(0x98cc) = 0;
    W(0x96d2) = 1;
    W(0x975e) = 2;
    W(0x9724) = 220;
    W(0x9726) = 1200;
    W(0x972c) = 0;
    W(0x9728) = 12;
    W(0x972a) = 50;
    W(0xa118) = 2;
    W(0xa128) = 2;
    W(0xa148) = (i16)(rnd(5) + 6);
    { int t; do t = rnd(8) - 1; while (t == 1 || t == 2 || t == 3 || t == 4 || t == 5 || t == 6); W(0xa158) = (i16)t; }   // initial target
}

static void init_egypt(void) {                    // 1D93:1693
    W(0x92f0) = 1075;
    W(0x9300) = 5000;
    W(0xa03e) = 35;
    W(0xa04e) = (i16)(rnd(3) + 1);
    W(0xa0fe) = (i16)(rnd(5) + 2);
    W(0x98c2) = 2;
    W(0x9416) = 1;
    W(0x943e) = 1;
    W(0x93ee) = 2;
    W(0x93a0) = 3000;
    W(0x93a2) = 2000;
    W(0x93a8) = 80;
    W(0x93a4) = 25;
    W(0x93a6) = 100;
    W(0xa10e) = 0;
    W(0xa11e) = 0;
    W(0xa13e) = (i16)(rnd(7) + -1);
    { int t; do t = rnd(8) - 1; while (t == 0 || t == 1 || t == 2 || t == 4 || t == 6); W(0xa14e) = (i16)t; }   // initial target
}

static void init_jordan(void) {                    // 1D93:1742
    W(0x92f6) = 100;
    W(0x9306) = 300;
    W(0xa044) = 15;
    W(0xa054) = (i16)(rnd(3) + 1);
    W(0xa104) = 0;
    W(0x98c8) = 1;
    W(0x95ba) = 1;
    W(0x960a) = 1;
    W(0x961e) = 1;
    W(0x9592) = 2;
    W(0x95e2) = 2;
    W(0x95bc) = 610;
    W(0x95be) = 200;
    W(0x95c4) = 320;
    W(0x95c0) = 3;
    W(0x95c2) = 60;
    W(0xa114) = 0;
    W(0xa124) = 0;
    W(0xa144) = (i16)(rnd(7) + -1);
    { int t; do t = rnd(8) - 1; while (t == 1 || t == 3 || t == 5); W(0xa154) = (i16)t; }   // initial target
}

static void init_lebanon(void) {                    // 1D93:17E5
    W(0x92f8) = 308;
    W(0x9308) = 1000;
    W(0xa046) = 2;
    W(0xa056) = 2;
    W(0xa106) = 0;
    W(0x98ca) = 0;
    W(0x96aa) = 1;
    W(0x96be) = 1;
    W(0x966e) = 2;
    W(0x9670) = 170;
    W(0x9672) = 0;
    W(0x9678) = 40;
    W(0x9674) = 3;
    W(0x9676) = 12;
    W(0xa116) = 3;
    W(0xa126) = 3;
    W(0xa146) = (i16)(rnd(13) + -1);
    { int t; do t = rnd(8) - 1; while (t == 0 || t == 1 || t == 2 || t == 3 || t == 4 || t == 5); W(0xa156) = (i16)t; }   // initial target
}

static void init_syria(void) {                    // 1D93:1878
    W(0x92fc) = 400;
    W(0x930c) = 1000;
    W(0xa04a) = 25;
    W(0xa05a) = (i16)(rnd(3) + 1);
    W(0xa10a) = (i16)(rnd(5) + 2);
    W(0x98ce) = 2;
    W(0x979a) = 1;
    W(0x97ae) = 1;
    W(0x97c2) = 1;
    W(0x97fe) = 1;
    W(0x9786) = 2;
    W(0x97d8) = 2000;
    W(0x97da) = 2000;
    W(0x97e0) = 300;
    W(0x97dc) = 8;
    W(0x97de) = 180;
    W(0xa12a) = 0;
    W(0xa11a) = 1;
    W(0xa14a) = (i16)(rnd(7) + -1);
    { int t; do t = rnd(8) - 1; while (t == 0 || t == 1 || t == 5 || t == 6); W(0xa15a) = (i16)t; }   // initial target
}

static void init_israel(void) {                    // 1D93:192F
    W(0x92fe) = 1050;
    W(0x930e) = 4500;
    W(0xa04c) = 35;
    W(0xa05c) = 4;
    W(0xa10c) = 6;
    W(0x98d0) = 3;
    W(0x9812) = 1;
    W(0x984e) = 1;
    W(0x988a) = 1;
    W(0x9862) = 1;
    W(0x988c) = 1400;
    W(0x988e) = 2000;
    W(0x9894) = 50;
    W(0x9890) = 5;
    W(0x9892) = 200;
    W(0xa11c) = 1;
    W(0xa12c) = 2;
    W(0xa14c) = 14;
    W(0xa15c) = 7;
}
