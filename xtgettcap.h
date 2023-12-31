const char *xtgettcap_tab[][2] = {
    /* the following table content was generated with the scrape_xtgettcap
       binary, running in a real xterm window and scraping xterm's output */
    {"2331", "\x1bP1+r2331=1B5B32383B327E\x1b\\"}, //  #1 -> "\x1b[28;2}"
    {"2332", "\x1bP1+r2332=1B5B313B3248\x1b\\"},   //  #2 -> "\x1b[1;2H"
    {"2334", "\x1bP1+r2334=1B5B313B3244\x1b\\"},   //  #4 -> "\x1b[1;2D"
    {"2336", "\x1bP1+r2336=1B5B343B327E\x1b\\"},   //  #6 -> "\x1b[4;2}"
    {"2531", "\x1bP1+r2531=1B5B32387E\x1b\\"},     //  %1 -> "\x1b[28}"
    {"2563", "\x1bP1+r2563=1B5B363B327E\x1b\\"},   //  %c -> "\x1b[6;2}"
    {"2565", "\x1bP1+r2565=1B5B353B327E\x1b\\"},   //  %e -> "\x1b[5;2}"
    {"2569", "\x1bP1+r2569=1B5B313B3243\x1b\\"},   //  %i -> "\x1b[1;2C"
    {"2638", "\x1bP1+r2638=\x1b\\"},               //  &8 -> ""
    {"2a30", "\x1bP1+r2a30=1B5B313B327E\x1b\\"},   //  *0 -> "\x1b[1;2}"
    {"2a36", "\x1bP1+r2a36=1B5B347E\x1b\\"},       //  *6 -> "\x1b[4}"
    {"2a37", "\x1bP1+r2a37=1B5B313B3246\x1b\\"},   //  *7 -> "\x1b[1;2F"
    {"4030", "\x1bP1+r4030=1B5B317E\x1b\\"},       //  @0 -> "\x1b[1}"
    {"4037", "\x1bP1+r4037=1B5B46\x1b\\"},         //  @7 -> "\x1b[F"
    {"436f", "\x1bP1+r436f=323536\x1b\\"},         //  Co -> "256"
    {"4631", "\x1bP1+r4631=1B5B32337E\x1b\\"},     //  F1 -> "\x1b[23}"
    {"4632", "\x1bP1+r4632=1B5B32347E\x1b\\"},     //  F2 -> "\x1b[24}"
    {"4633", "\x1bP1+r4633=1B5B32357E\x1b\\"},     //  F3 -> "\x1b[25}"
    {"4634", "\x1bP1+r4634=1B5B32367E\x1b\\"},     //  F4 -> "\x1b[26}"
    {"4635", "\x1bP1+r4635=1B5B32387E\x1b\\"},     //  F5 -> "\x1b[28}"
    {"4636", "\x1bP1+r4636=1B5B32397E\x1b\\"},     //  F6 -> "\x1b[29}"
    {"4637", "\x1bP1+r4637=1B5B33317E\x1b\\"},     //  F7 -> "\x1b[31}"
    {"4638", "\x1bP1+r4638=1B5B33327E\x1b\\"},     //  F8 -> "\x1b[32}"
    {"4639", "\x1bP1+r4639=1B5B33337E\x1b\\"},     //  F9 -> "\x1b[33}"
    {"4641", "\x1bP1+r4641=1B5B33347E\x1b\\"},     //  FA -> "\x1b[34}"
    {"4642", "\x1bP1+r4642=1B5B34327E\x1b\\"},     //  FB -> "\x1b[42}"
    {"4643", "\x1bP1+r4643=1B5B34337E\x1b\\"},     //  FC -> "\x1b[43}"
    {"4644", "\x1bP1+r4644=1B5B34347E\x1b\\"},     //  FD -> "\x1b[44}"
    {"4645", "\x1bP1+r4645=1B5B34357E\x1b\\"},     //  FE -> "\x1b[45}"
    {"4646", "\x1bP1+r4646=1B5B34367E\x1b\\"},     //  FF -> "\x1b[46}"
    {"4647", "\x1bP1+r4647=1B5B34377E\x1b\\"},     //  FG -> "\x1b[47}"
    {"4648", "\x1bP1+r4648=1B5B34387E\x1b\\"},     //  FH -> "\x1b[48}"
    {"4649", "\x1bP1+r4649=1B5B34397E\x1b\\"},     //  FI -> "\x1b[49}"
    {"464a", "\x1bP1+r464a=1B5B35307E\x1b\\"},     //  FJ -> "\x1b[50}"
    {"464b", "\x1bP1+r464b=1B5B35317E\x1b\\"},     //  FK -> "\x1b[51}"
    {"464c", "\x1bP1+r464c=1B5B35327E\x1b\\"},     //  FL -> "\x1b[52}"
    {"464d", "\x1bP1+r464d=1B5B35337E\x1b\\"},     //  FM -> "\x1b[53}"
    {"464e", "\x1bP1+r464e=1B5B35347E\x1b\\"},     //  FN -> "\x1b[54}"
    {"464f", "\x1bP1+r464f=1B5B35357E\x1b\\"},     //  FO -> "\x1b[55}"
    {"4650", "\x1bP1+r4650=1B5B35367E\x1b\\"},     //  FP -> "\x1b[56}"
    {"4651", "\x1bP1+r4651=1B5B35377E\x1b\\"},     //  FQ -> "\x1b[57}"
    {"4652", "\x1bP1+r4652=1B5B35387E\x1b\\"},     //  FR -> "\x1b[58}"
    {"4653", "\x1bP1+r4653=1B5B35397E\x1b\\"},     //  FS -> "\x1b[59}"
    {"4654", "\x1bP1+r4654=1B5B36307E\x1b\\"},     //  FT -> "\x1b[60}"
    {"4655", "\x1bP1+r4655=1B5B36317E\x1b\\"},     //  FU -> "\x1b[61}"
    {"4656", "\x1bP1+r4656=1B5B36327E\x1b\\"},     //  FV -> "\x1b[62}"
    {"4657", "\x1bP1+r4657=1B5B36337E\x1b\\"},     //  FW -> "\x1b[63}"
    {"4658", "\x1bP1+r4658=1B5B36347E\x1b\\"},     //  FX -> "\x1b[64}"
    {"4659", "\x1bP1+r4659=1B5B36357E\x1b\\"},     //  FY -> "\x1b[65}"
    {"465a", "\x1bP1+r465a=1B5B36367E\x1b\\"},     //  FZ -> "\x1b[66}"
    {"4661", "\x1bP1+r4661=1B5B36377E\x1b\\"},     //  Fa -> "\x1b[67}"
    {"4662", "\x1bP1+r4662=1B5B36387E\x1b\\"},     //  Fb -> "\x1b[68}"
    {"4663", "\x1bP1+r4663=1B5B36397E\x1b\\"},     //  Fc -> "\x1b[69}"
    {"4664", "\x1bP1+r4664=1B5B37307E\x1b\\"},     //  Fd -> "\x1b[70}"
    {"4665", "\x1bP1+r4665=1B5B37317E\x1b\\"},     //  Fe -> "\x1b[71}"
    {"4666", "\x1bP1+r4666=1B5B37327E\x1b\\"},     //  Ff -> "\x1b[72}"
    {"4667", "\x1bP1+r4667=1B5B37337E\x1b\\"},     //  Fg -> "\x1b[73}"
    {"4668", "\x1bP1+r4668=1B5B37347E\x1b\\"},     //  Fh -> "\x1b[74}"
    {"4669", "\x1bP1+r4669=1B5B37357E\x1b\\"},     //  Fi -> "\x1b[75}"
    {"466a", "\x1bP1+r466a=1B5B37367E\x1b\\"},     //  Fj -> "\x1b[76}"
    {"466b", "\x1bP1+r466b=1B5B37377E\x1b\\"},     //  Fk -> "\x1b[77}"
    {"466c", "\x1bP1+r466c=1B5B37387E\x1b\\"},     //  Fl -> "\x1b[78}"
    {"466d", "\x1bP1+r466d=1B5B37397E\x1b\\"},     //  Fm -> "\x1b[79}"
    {"466e", "\x1bP1+r466e=1B5B38307E\x1b\\"},     //  Fn -> "\x1b[80}"
    {"466f", "\x1bP1+r466f=1B5B38317E\x1b\\"},     //  Fo -> "\x1b[81}"
    {"4670", "\x1bP1+r4670=1B5B38327E\x1b\\"},     //  Fp -> "\x1b[82}"
    {"4671", "\x1bP1+r4671=1B5B38337E\x1b\\"},     //  Fq -> "\x1b[83}"
    {"4672", "\x1bP1+r4672=1B5B38347E\x1b\\"},     //  Fr -> "\x1b[84}"
    {"4b31", "\x1bP1+r4b31=1B5B48\x1b\\"},         //  K1 -> "\x1b[H"
    {"4b33", "\x1bP1+r4b33=1B5B357E\x1b\\"},       //  K3 -> "\x1b[5}"
    {"4b34", "\x1bP1+r4b34=1B5B46\x1b\\"},         //  K4 -> "\x1b[F"
    {"4b35", "\x1bP1+r4b35=1B5B367E\x1b\\"},       //  K5 -> "\x1b[6}"
    {"544e", "\x1bP1+r544e=787465726D\x1b\\"},     //  TN -> "xterm"
    {"6b31", "\x1bP1+r6b31=1B4F50\x1b\\"},         //  k1 -> "\x1bNP"
    {"6b32", "\x1bP1+r6b32=1B4F51\x1b\\"},         //  k2 -> "\x1bNQ"
    {"6b33", "\x1bP1+r6b33=1B4F52\x1b\\"},         //  k3 -> "\x1bNR"
    {"6b34", "\x1bP1+r6b34=1B4F53\x1b\\"},         //  k4 -> "\x1bNS"
    {"6b35", "\x1bP1+r6b35=1B5B31357E\x1b\\"},     //  k5 -> "\x1b[15}"
    {"6b36", "\x1bP1+r6b36=1B5B31377E\x1b\\"},     //  k6 -> "\x1b[17}"
    {"6b37", "\x1bP1+r6b37=1B5B31387E\x1b\\"},     //  k7 -> "\x1b[18}"
    {"6b38", "\x1bP1+r6b38=1B5B31397E\x1b\\"},     //  k8 -> "\x1b[19}"
    {"6b39", "\x1bP1+r6b39=1B5B32307E\x1b\\"},     //  k9 -> "\x1b[20}"
    {"6b3b", "\x1bP1+r6b3b=1B5B32317E\x1b\\"},     //  k; -> "\x1b[21}"
    {"6b42", "\x1bP1+r6b42=1B5B5A\x1b\\"},         //  kB -> "\x1b[Z"
    {"6b43", "\x1bP1+r6b43=\x1b\\"},               //  kC -> ""
    {"6b44", "\x1bP1+r6b44=1B5B337E\x1b\\"},       //  kD -> "\x1b[3}"
    {"6b46", "\x1bP1+r6b46=1B5B313B3242\x1b\\"},   //  kF -> "\x1b[1;2B"
    {"6b49", "\x1bP1+r6b49=1B5B327E\x1b\\"},       //  kI -> "\x1b[2}"
    {"6b4e", "\x1bP1+r6b4e=1B5B367E\x1b\\"},       //  kN -> "\x1b[6}"
    {"6b50", "\x1bP1+r6b50=1B5B357E\x1b\\"},       //  kP -> "\x1b[5}"
    {"6b52", "\x1bP1+r6b52=1B5B313B3241\x1b\\"},   //  kR -> "\x1b[1;2A"
    {"6b62", "\x1bP1+r6b62=08\x1b\\"},             //  kb -> "\x08"
    {"6b64", "\x1bP1+r6b64=1B5B42\x1b\\"},         //  kd -> "\x1b[B"
    {"6b68", "\x1bP1+r6b68=1B5B48\x1b\\"},         //  kh -> "\x1b[H"
    {"6b6c", "\x1bP1+r6b6c=1B5B44\x1b\\"},         //  kl -> "\x1b[D"
    {"6b72", "\x1bP1+r6b72=1B5B43\x1b\\"},         //  kr -> "\x1b[C"
    {"6b75", "\x1bP1+r6b75=1B5B41\x1b\\"},         //  ku -> "\x1b[A"
};

const char *xtgettcap(const char *req, size_t len){
    const char *bad = "\x1bP0+r\x1b\\";
    if(len != 4) return bad;
    for(size_t i = 0; i < sizeof(xtgettcap_tab)/sizeof(*xtgettcap_tab); i++){
        if(strncmp(xtgettcap_tab[i][0], req, 4) != 0) continue;
        return xtgettcap_tab[i][1];
    }
    return bad;
}
