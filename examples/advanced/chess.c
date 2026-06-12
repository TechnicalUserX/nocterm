/**
 * @file chess.c
 *
 * A complete two-player chess game built on the Nocterm widget library, with an
 * optional "versus computer" mode driven by a small built-in engine.
 *
 * Design notes
 * ------------
 * No external chess engine is bundled: a standalone example must build with a
 * single `gcc` invocation and not depend on an external binary such as
 * Stockfish.  Since full legal-move generation is required anyway (to detect
 * check / checkmate / stalemate), the same generator powers a compact
 * alpha-beta search for the computer opponent.  The move generator was
 * validated separately with perft (start position to depth 5 and the
 * "Kiwipete" position to depth 4, both matching the known reference counts).
 *
 * Board orientation in memory: row 0 = rank 8 (black's back rank), row 7 =
 * rank 1 (white's back rank); white is drawn at the bottom.  White piece codes
 * are positive and move toward row 0; black codes are negative.
 *
 * Controls: arrow keys move the cursor, Enter/Space selects a piece and then a
 * destination, on promotion press Q/R/B/N, 'u' undoes, 'n' returns to the menu,
 * 'q' quits.
 */

#include <nocterm/nocterm.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

/* ------------------------------------------------------------------ */
/* Chess core (validated with perft)                                  */
/* ------------------------------------------------------------------ */

enum { PAWN=1, KNIGHT=2, BISHOP=3, ROOK=4, QUEEN=5, KING=6 };
enum { CF_CAPTURE=1, CF_DOUBLE=2, CF_EP=4, CF_CASTLE_K=8, CF_CASTLE_Q=16 };
enum { CR_WK=1, CR_WQ=2, CR_BK=4, CR_BQ=8 };

typedef struct { int8_t b[8][8]; int8_t side; uint8_t castle; int8_t ep_r, ep_c; int halfmove, fullmove; } pos_t;
typedef struct { int8_t fr,fc,tr,tc; int8_t promo; uint8_t flag; } move_t;

static void pos_startpos(pos_t* p){
    memset(p,0,sizeof(*p));
    int8_t back[8] = {ROOK,KNIGHT,BISHOP,QUEEN,KING,BISHOP,KNIGHT,ROOK};
    for(int c=0;c<8;c++){
        p->b[0][c] = -back[c];
        p->b[1][c] = -PAWN;
        p->b[6][c] =  PAWN;
        p->b[7][c] =  back[c];
    }
    p->side = 1;
    p->castle = CR_WK|CR_WQ|CR_BK|CR_BQ;
    p->ep_r = -1; p->ep_c = -1;
    p->fullmove = 1;
}

static inline int on(int r,int c){ return r>=0&&r<8&&c>=0&&c<8; }

static int attacked(const pos_t* p, int r, int c, int byside){
    if(byside>0){
        if(on(r+1,c-1) && p->b[r+1][c-1]==PAWN) return 1;
        if(on(r+1,c+1) && p->b[r+1][c+1]==PAWN) return 1;
    }else{
        if(on(r-1,c-1) && p->b[r-1][c-1]==-PAWN) return 1;
        if(on(r-1,c+1) && p->b[r-1][c+1]==-PAWN) return 1;
    }
    static const int kn[8][2]={{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    for(int i=0;i<8;i++){ int rr=r+kn[i][0],cc=c+kn[i][1]; if(on(rr,cc)&&p->b[rr][cc]==byside*KNIGHT) return 1; }
    for(int dr=-1;dr<=1;dr++)for(int dc=-1;dc<=1;dc++){ if(!dr&&!dc)continue; int rr=r+dr,cc=c+dc; if(on(rr,cc)&&p->b[rr][cc]==byside*KING) return 1; }
    static const int di[4][2]={{-1,-1},{-1,1},{1,-1},{1,1}};
    for(int i=0;i<4;i++){ int rr=r+di[i][0],cc=c+di[i][1];
        while(on(rr,cc)){ int8_t pc=p->b[rr][cc]; if(pc){ if(pc==byside*BISHOP||pc==byside*QUEEN) return 1; break; } rr+=di[i][0]; cc+=di[i][1]; } }
    static const int orth[4][2]={{-1,0},{1,0},{0,-1},{0,1}};
    for(int i=0;i<4;i++){ int rr=r+orth[i][0],cc=c+orth[i][1];
        while(on(rr,cc)){ int8_t pc=p->b[rr][cc]; if(pc){ if(pc==byside*ROOK||pc==byside*QUEEN) return 1; break; } rr+=orth[i][0]; cc+=orth[i][1]; } }
    return 0;
}

static void find_king(const pos_t* p, int side, int* kr, int* kc){
    for(int r=0;r<8;r++)for(int c=0;c<8;c++) if(p->b[r][c]==side*KING){ *kr=r;*kc=c; return; }
    *kr=-1;*kc=-1;
}

static int in_check(const pos_t* p, int side){
    int kr,kc; find_king(p,side,&kr,&kc); if(kr<0) return 0; return attacked(p,kr,kc,-side);
}

static void make_move(pos_t* p, const move_t* m){
    int8_t piece = p->b[m->fr][m->fc];
    int side = p->side;
    p->ep_r = -1; p->ep_c = -1;
    if(m->flag & CF_EP){ p->b[m->fr][m->tc] = 0; }
    p->b[m->tr][m->tc] = piece;
    p->b[m->fr][m->fc] = 0;
    if(m->promo){ p->b[m->tr][m->tc] = side*m->promo; }
    if(m->flag & CF_DOUBLE){ p->ep_r = (m->fr+m->tr)/2; p->ep_c = m->fc; }
    if(m->flag & CF_CASTLE_K){ p->b[m->fr][5]=p->b[m->fr][7]; p->b[m->fr][7]=0; }
    if(m->flag & CF_CASTLE_Q){ p->b[m->fr][3]=p->b[m->fr][0]; p->b[m->fr][0]=0; }
    if(piece==KING){ p->castle &= ~(CR_WK|CR_WQ); }
    if(piece==-KING){ p->castle &= ~(CR_BK|CR_BQ); }
    if((m->fr==7&&m->fc==7)||(m->tr==7&&m->tc==7)) p->castle &= ~CR_WK;
    if((m->fr==7&&m->fc==0)||(m->tr==7&&m->tc==0)) p->castle &= ~CR_WQ;
    if((m->fr==0&&m->fc==7)||(m->tr==0&&m->tc==7)) p->castle &= ~CR_BK;
    if((m->fr==0&&m->fc==0)||(m->tr==0&&m->tc==0)) p->castle &= ~CR_BQ;
    if(piece==PAWN || piece==-PAWN || (m->flag&CF_CAPTURE)) p->halfmove=0; else p->halfmove++;
    if(side<0) p->fullmove++;
    p->side = -side;
}

static int gen_pseudo(const pos_t* p, move_t* list){
    int n=0; int side=p->side;
    for(int r=0;r<8;r++)for(int c=0;c<8;c++){
        int8_t pc=p->b[r][c]; if(pc==0 || (pc>0)!=(side>0)) continue;
        int t=pc>0?pc:-pc;
        if(t==PAWN){
            int dir = side>0? -1: 1;
            int startrow = side>0? 6:1;
            int promorow = side>0? 0:7;
            int nr=r+dir;
            if(on(nr,c) && p->b[nr][c]==0){
                if(nr==promorow){ int8_t pr[4]={QUEEN,ROOK,BISHOP,KNIGHT}; for(int i=0;i<4;i++){ list[n++] = (move_t){r,c,nr,c,pr[i],0}; } }
                else list[n++] = (move_t){r,c,nr,c,0,0};
                if(r==startrow && p->b[r+2*dir][c]==0){ list[n++] = (move_t){r,c,r+2*dir,c,0,CF_DOUBLE}; }
            }
            for(int dc=-1;dc<=1;dc+=2){ int cc=c+dc; if(!on(nr,cc))continue;
                int8_t tp=p->b[nr][cc];
                if(tp!=0 && (tp>0)!=(side>0)){
                    if(nr==promorow){ int8_t pr[4]={QUEEN,ROOK,BISHOP,KNIGHT}; for(int i=0;i<4;i++){ list[n++]=(move_t){r,c,nr,cc,pr[i],CF_CAPTURE}; } }
                    else list[n++]=(move_t){r,c,nr,cc,0,CF_CAPTURE};
                } else if(p->ep_r==nr && p->ep_c==cc){
                    list[n++]=(move_t){r,c,nr,cc,0,CF_CAPTURE|CF_EP};
                }
            }
        } else if(t==KNIGHT){
            static const int kn[8][2]={{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
            for(int i=0;i<8;i++){ int rr=r+kn[i][0],cc=c+kn[i][1]; if(!on(rr,cc))continue; int8_t tp=p->b[rr][cc]; if(tp==0) list[n++]=(move_t){r,c,rr,cc,0,0}; else if((tp>0)!=(side>0)) list[n++]=(move_t){r,c,rr,cc,0,CF_CAPTURE}; }
        } else if(t==KING){
            for(int dr=-1;dr<=1;dr++)for(int dc=-1;dc<=1;dc++){ if(!dr&&!dc)continue; int rr=r+dr,cc=c+dc; if(!on(rr,cc))continue; int8_t tp=p->b[rr][cc]; if(tp==0) list[n++]=(move_t){r,c,rr,cc,0,0}; else if((tp>0)!=(side>0)) list[n++]=(move_t){r,c,rr,cc,0,CF_CAPTURE}; }
            int home = side>0?7:0;
            if(r==home && c==4 && !attacked(p,home,4,-side)){
                uint8_t kbit = side>0?CR_WK:CR_BK, qbit = side>0?CR_WQ:CR_BQ;
                if((p->castle&kbit) && p->b[home][5]==0 && p->b[home][6]==0 && p->b[home][7]==side*ROOK
                   && !attacked(p,home,5,-side) && !attacked(p,home,6,-side))
                    list[n++]=(move_t){r,c,home,6,0,CF_CASTLE_K};
                if((p->castle&qbit) && p->b[home][3]==0 && p->b[home][2]==0 && p->b[home][1]==0 && p->b[home][0]==side*ROOK
                   && !attacked(p,home,3,-side) && !attacked(p,home,2,-side))
                    list[n++]=(move_t){r,c,home,2,0,CF_CASTLE_Q};
            }
        } else {
            int dirs[8][2]; int nd=0;
            if(t==BISHOP||t==QUEEN){ int d[4][2]={{-1,-1},{-1,1},{1,-1},{1,1}}; for(int i=0;i<4;i++){dirs[nd][0]=d[i][0];dirs[nd][1]=d[i][1];nd++;} }
            if(t==ROOK||t==QUEEN){ int d[4][2]={{-1,0},{1,0},{0,-1},{0,1}}; for(int i=0;i<4;i++){dirs[nd][0]=d[i][0];dirs[nd][1]=d[i][1];nd++;} }
            for(int i=0;i<nd;i++){ int rr=r+dirs[i][0],cc=c+dirs[i][1];
                while(on(rr,cc)){ int8_t tp=p->b[rr][cc]; if(tp==0){ list[n++]=(move_t){r,c,rr,cc,0,0}; } else { if((tp>0)!=(side>0)) list[n++]=(move_t){r,c,rr,cc,0,CF_CAPTURE}; break; } rr+=dirs[i][0]; cc+=dirs[i][1]; } }
        }
    }
    return n;
}

static int gen_legal(const pos_t* p, move_t* out){
    move_t tmp[256]; int n=gen_pseudo(p,tmp); int m=0; int side=p->side;
    for(int i=0;i<n;i++){ pos_t cp=*p; make_move(&cp,&tmp[i]); if(!in_check(&cp, side)) out[m++]=tmp[i]; }
    return m;
}

/* ------------------------------------------------------------------ */
/* Engine: alpha-beta search with a simplified evaluation             */
/* ------------------------------------------------------------------ */

#define BOT_DEPTH 3
#define MATE_SCORE 1000000

static const int piece_value[7] = { 0, 100, 320, 330, 500, 900, 0 };

/* Piece-square tables, white's perspective, row 0 = rank 8 (Michniewski). */
static const int pst_pawn[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0},
    { 50, 50, 50, 50, 50, 50, 50, 50},
    { 10, 10, 20, 30, 30, 20, 10, 10},
    {  5,  5, 10, 25, 25, 10,  5,  5},
    {  0,  0,  0, 20, 20,  0,  0,  0},
    {  5, -5,-10,  0,  0,-10, -5,  5},
    {  5, 10, 10,-20,-20, 10, 10,  5},
    {  0,  0,  0,  0,  0,  0,  0,  0}
};
static const int pst_knight[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};
static const int pst_bishop[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5, 10, 10,  5,  0,-10},
    {-10,  5,  5, 10, 10,  5,  5,-10},
    {-10,  0, 10, 10, 10, 10,  0,-10},
    {-10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  5,  0,  0,  0,  0,  5,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};
static const int pst_rook[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0},
    {  5, 10, 10, 10, 10, 10, 10,  5},
    { -5,  0,  0,  0,  0,  0,  0, -5},
    { -5,  0,  0,  0,  0,  0,  0, -5},
    { -5,  0,  0,  0,  0,  0,  0, -5},
    { -5,  0,  0,  0,  0,  0,  0, -5},
    { -5,  0,  0,  0,  0,  0,  0, -5},
    {  0,  0,  0,  5,  5,  0,  0,  0}
};
static const int pst_queen[8][8] = {
    {-20,-10,-10, -5, -5,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5,  5,  5,  5,  0,-10},
    { -5,  0,  5,  5,  5,  5,  0, -5},
    {  0,  0,  5,  5,  5,  5,  0, -5},
    {-10,  5,  5,  5,  5,  5,  0,-10},
    {-10,  0,  5,  0,  0,  0,  0,-10},
    {-20,-10,-10, -5, -5,-10,-10,-20}
};
static const int pst_king[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    { 20, 20,  0,  0,  0,  0, 20, 20},
    { 20, 30, 10,  0,  0, 10, 30, 20}
};

static int pst_lookup(int type, int r, int c){
    switch(type){
        case PAWN:   return pst_pawn[r][c];
        case KNIGHT: return pst_knight[r][c];
        case BISHOP: return pst_bishop[r][c];
        case ROOK:   return pst_rook[r][c];
        case QUEEN:  return pst_queen[r][c];
        case KING:   return pst_king[r][c];
    }
    return 0;
}

/* Static evaluation from white's perspective (positive favours white). */
static int evaluate(const pos_t* p){
    int score = 0;
    for(int r=0;r<8;r++)for(int c=0;c<8;c++){
        int8_t pc = p->b[r][c]; if(!pc) continue;
        int t = pc>0?pc:-pc;
        if(pc>0) score += piece_value[t] + pst_lookup(t, r, c);
        else     score -= piece_value[t] + pst_lookup(t, 7-r, c); /* mirror for black */
    }
    return score;
}

/* Order captures (and promotions) first to improve alpha-beta pruning.
 * Selection sort is fine here: the move list is short. */
static void order_moves(const pos_t* p, move_t* list, int n){
    for(int i=0;i<n;i++){
        int best=i; int bestv=-1;
        for(int k=i;k<n;k++){
            int8_t victim = p->b[list[k].tr][list[k].tc];
            int s=0;
            if(list[k].flag & CF_CAPTURE){ int v=victim>0?victim:-victim; s=10*piece_value[v]+10000; }
            if(list[k].promo) s+=piece_value[list[k].promo];
            if(s>bestv){ bestv=s; best=k; }
        }
        move_t tmp=list[i]; list[i]=list[best]; list[best]=tmp;
    }
}

static int negamax(const pos_t* p, int depth, int alpha, int beta){
    move_t list[256];
    int n = gen_legal(p, list);
    if(n==0){
        if(in_check(p, p->side)) return -MATE_SCORE + (BOT_DEPTH - depth); /* prefer slower mates */
        return 0; /* stalemate */
    }
    if(depth==0){
        int e = evaluate(p);
        return p->side>0 ? e : -e;
    }
    order_moves(p, list, n);
    int best = -2*MATE_SCORE;
    for(int i=0;i<n;i++){
        pos_t cp=*p; make_move(&cp,&list[i]);
        int v = -negamax(&cp, depth-1, -beta, -alpha);
        if(v>best) best=v;
        if(best>alpha) alpha=best;
        if(alpha>=beta) break; /* beta cutoff */
    }
    return best;
}

/* Choose the best move for the side to move; returns 0 if no legal move. */
static int engine_best_move(const pos_t* p, move_t* out){
    move_t list[256];
    int n = gen_legal(p, list);
    if(n==0) return 0;
    order_moves(p, list, n);
    int best = -2*MATE_SCORE, alpha=-2*MATE_SCORE, beta=2*MATE_SCORE;
    int best_idx = 0;
    for(int i=0;i<n;i++){
        pos_t cp=*p; make_move(&cp,&list[i]);
        int v = -negamax(&cp, BOT_DEPTH-1, -beta, -alpha);
        if(v>best){ best=v; best_idx=i; }
        if(best>alpha) alpha=best;
    }
    *out = list[best_idx];
    return 1;
}

/* ------------------------------------------------------------------ */
/* Game / UI state                                                    */
/* ------------------------------------------------------------------ */

typedef enum { STATE_MENU, STATE_PLAY } ui_state_t;
typedef enum { MODE_TWO_PLAYER, MODE_VS_BOT } game_mode_t;
typedef enum { RESULT_NONE, RESULT_CHECKMATE, RESULT_STALEMATE, RESULT_DRAW } result_t;

#define BOARD_X 3   /* board squares start at this column (cols 0..1 = rank labels) */
#define BOARD_Y 1   /* board squares start at this row    (row 0 = file labels)    */
#define SQ_W    3
#define HIST_MAX 600

typedef struct {
    pos_t pos;
    ui_state_t state;
    game_mode_t mode;
    int bot_side;                 /* +1 / -1 : which colour the computer plays */

    move_t legal[256];            /* legal moves for the side to move */
    int legal_count;

    int cur_r, cur_c;             /* cursor square (board coordinates) */
    int sel_r, sel_c;             /* selected source square, -1 if none */

    int promo_pending;            /* awaiting promotion piece choice */
    move_t promo_from_to;         /* from/to of the pending promotion */

    int last_fr, last_fc, last_tr, last_tc; /* last move for highlight, -1 if none */

    result_t result;
    int in_check_now;

    pos_t hist[HIST_MAX];         /* snapshots for undo */
    int hist_len;

    char message[96];

    nocterm_widget_t* board;
    nocterm_widget_t* status;
} chess_t;

static chess_t g;

/* ------------------------------------------------------------------ */
/* Rendering helpers                                                  */
/* ------------------------------------------------------------------ */

static nocterm_attribute_t attr_rgb(int br,int bg,int bb,int fr,int fg,int fb,int bold){
    nocterm_attribute_t a = {0};
    a.color.rgb.bg = true; a.color.rgb.codes.bg.red=br; a.color.rgb.codes.bg.green=bg; a.color.rgb.codes.bg.blue=bb;
    a.color.rgb.fg = true; a.color.rgb.codes.fg.red=fr; a.color.rgb.codes.fg.green=fg; a.color.rgb.codes.fg.blue=fb;
    a.bold = bold?1:0;
    return a;
}

static nocterm_char_t piece_glyph(int type){
    /* Filled chess glyphs U+265A..U+265F; colour conveys the side. */
    static const char* gl[7] = { " ", "\xe2\x99\x9f" /*pawn*/, "\xe2\x99\x9e" /*knight*/,
        "\xe2\x99\x9d" /*bishop*/, "\xe2\x99\x9c" /*rook*/, "\xe2\x99\x9b" /*queen*/, "\xe2\x99\x9a" /*king*/ };
    nocterm_char_t ch = {0};
    if(type<=0){ return nocterm_char_from_ascii(' '); }
    ch.is_utf8 = true; ch.bytes_size = 3;
    memcpy(ch.bytes, gl[type], 3);
    return ch;
}

static void draw_text(nocterm_widget_t* w, int row, int col, const char* s, nocterm_attribute_t a){
    nocterm_char_t buf[128];
    uint64_t len = nocterm_char_string_from_stream(buf, 128, s, strlen(s));
    for(uint64_t i=0;i<len;i++) nocterm_widget_update(w, row, col+(int)i, buf[i], a);
}

/* Absolute screen origin of a widget = sum of bounds offsets up to the root. */
static void widget_abs(nocterm_widget_t* w, int* ar, int* ac){
    int r=0,c=0;
    for(nocterm_widget_t* it=w; it; it=it->parent){ r+=it->bounds.row; c+=it->bounds.col; }
    *ar=r; *ac=c;
}

/* Map an absolute click (mr,mc) to a board square; returns 1 and fills r,c on a hit. */
static int click_to_square(int mr, int mc, int* r, int* c){
    int abr, abc; widget_abs(g.board, &abr, &abc);
    int rr = mr - abr - BOARD_Y;
    int cc = mc - abc - BOARD_X;
    if(rr<0 || rr>=8 || cc<0 || cc>=8*SQ_W) return 0;
    *r = rr; *c = cc / SQ_W;
    return 1;
}

/* Map an absolute click to a menu row local to the board widget (-1 if none). */
static int click_to_menu_row(int mr){
    int abr, abc; widget_abs(g.board, &abr, &abc); (void)abc;
    return mr - abr;
}

/* Is (r,c) a legal destination from the currently selected square? */
static int is_target(int r,int c){
    if(g.sel_r<0) return 0;
    for(int i=0;i<g.legal_count;i++)
        if(g.legal[i].fr==g.sel_r && g.legal[i].fc==g.sel_c && g.legal[i].tr==r && g.legal[i].tc==c) return 1;
    return 0;
}

static void render_square(int r,int c){
    int light = ((r+c)&1)==0;
    int br,bg,bb;
    if(light){ br=235; bg=209; bb=166; } else { br=165; bg=117; bb=80; }

    int is_sel    = (r==g.sel_r && c==g.sel_c);
    int is_cur    = (r==g.cur_r && c==g.cur_c);
    int is_last   = (r==g.last_fr && c==g.last_fc) || (r==g.last_tr && c==g.last_tc);
    int tgt       = is_target(r,c);
    int8_t pc     = g.pos.b[r][c];
    int king_chk  = g.in_check_now && pc==g.pos.side*KING;

    /* Square colour precedence: check > cursor > selected > target > last move. */
    if(king_chk){ br=200; bg=70;  bb=70;  }
    else if(is_cur){ br=246; bg=222; bb=110; }
    else if(is_sel){ br=130; bg=190; bb=120; }
    else if(tgt){ if(pc){ br=205; bg=110; bb=95; } else { br=light?205:150; bg=light?225:185; bb=light?175:130; } }
    else if(is_last){ if(light){ br=225; bg=222; bb=120; } else { br=170; bg=150; bb=70; } }

    int fr = pc>0 ? 250:25, fgc = pc>0 ? 250:25, fb = pc>0 ? 250:25;
    nocterm_attribute_t a = attr_rgb(br,bg,bb, fr,fgc,fb, pc!=0);

    int sr = BOARD_Y + r;
    int sc = BOARD_X + c*SQ_W;

    nocterm_char_t blank = nocterm_char_from_ascii(' ');
    nocterm_char_t mid;
    if(pc){ mid = piece_glyph(pc>0?pc:-pc); }
    else if(tgt){ mid.is_utf8=true; mid.bytes_size=3; memcpy(mid.bytes,"\xe2\x80\xa2",3); } /* bullet for empty target */
    else { mid = blank; }

    nocterm_widget_update(g.board, sr, sc,   blank, a);
    nocterm_widget_update(g.board, sr, sc+1, mid,   a);
    nocterm_widget_update(g.board, sr, sc+2, blank, a);
}

static void render_board(void){
    nocterm_widget_clear(g.board);
    nocterm_attribute_t lab = attr_rgb(0,0,0, 180,180,180, 0);
    /* file labels top and bottom */
    for(int c=0;c<8;c++){
        char f[2] = { (char)('a'+c), 0 };
        draw_text(g.board, 0, BOARD_X + c*SQ_W + 1, f, lab);
        draw_text(g.board, BOARD_Y + 8, BOARD_X + c*SQ_W + 1, f, lab);
    }
    /* rank labels + squares */
    for(int r=0;r<8;r++){
        char rk[2] = { (char)('8'-r), 0 };
        draw_text(g.board, BOARD_Y + r, 1, rk, lab);
        for(int c=0;c<8;c++) render_square(r,c);
    }
}

static const char* square_name(int r,int c, char* out){
    out[0]=(char)('a'+c); out[1]=(char)('8'-r); out[2]=0; return out;
}

static void render_status(void){
    nocterm_widget_clear(g.status);
    nocterm_attribute_t title = attr_rgb(0,0,0, 120,200,255, 1);
    nocterm_attribute_t norm  = attr_rgb(0,0,0, 220,220,220, 0);
    nocterm_attribute_t warn  = attr_rgb(0,0,0, 255,120,120, 1);
    nocterm_attribute_t good  = attr_rgb(0,0,0, 140,220,140, 1);
    nocterm_attribute_t dim   = attr_rgb(0,0,0, 140,140,140, 0);

    int row=0;
    draw_text(g.status, row++, 0, "C H E S S", title);
    row++;

    if(g.mode==MODE_TWO_PLAYER) draw_text(g.status, row++, 0, "Mode: Two players", norm);
    else { char m[40]; sprintf(m,"Mode: vs Computer (%s)", g.bot_side<0?"you = White":"you = Black"); draw_text(g.status, row++, 0, m, norm); }

    const char* turn = g.pos.side>0 ? "White" : "Black";
    char t[48]; sprintf(t, "Turn: %s   (move %d)", turn, g.pos.fullmove);
    draw_text(g.status, row++, 0, t, norm);
    row++;

    if(g.result==RESULT_CHECKMATE){
        char b[48]; sprintf(b, "CHECKMATE - %s wins!", g.pos.side>0?"Black":"White");
        draw_text(g.status, row++, 0, b, good);
    } else if(g.result==RESULT_STALEMATE){
        draw_text(g.status, row++, 0, "STALEMATE - draw", good);
    } else if(g.result==RESULT_DRAW){
        draw_text(g.status, row++, 0, "DRAW", good);
    } else if(g.promo_pending){
        draw_text(g.status, row++, 0, "Promote: [Q] [R] [B] [N]", warn);
    } else if(g.in_check_now){
        draw_text(g.status, row++, 0, "CHECK!", warn);
    } else {
        row++;
    }
    row++;

    if(g.message[0]) draw_text(g.status, row++, 0, g.message, dim);
    row++;

    draw_text(g.status, row++, 0, "Arrows: move cursor", dim);
    draw_text(g.status, row++, 0, "Enter/Space: select / move", dim);
    draw_text(g.status, row++, 0, "U: undo   N: new game", dim);
    draw_text(g.status, row++, 0, "Q: quit", dim);
}

static void render_menu(void){
    nocterm_widget_clear(g.board);
    nocterm_widget_clear(g.status);
    nocterm_attribute_t title = attr_rgb(0,0,0, 120,200,255, 1);
    nocterm_attribute_t norm  = attr_rgb(0,0,0, 220,220,220, 0);
    nocterm_attribute_t dim   = attr_rgb(0,0,0, 150,150,150, 0);
    /* Lines must fit the board widget width (30 cols) to avoid wrapping. */
    int row=1;
    draw_text(g.board, row++, 2, "C H E S S", title); row++;
    draw_text(g.board, row++, 2, "Choose a game mode:", norm); row++;
    draw_text(g.board, row++, 4, "[1]  Two players", norm);
    draw_text(g.board, row++, 4, "[2]  vs Computer (White)", norm);
    draw_text(g.board, row++, 4, "[3]  vs Computer (Black)", norm);
    draw_text(g.board, row++, 4, "[Q]  Quit", norm);
    row++;
    draw_text(g.board, row++, 2, "Two-player terminal chess.", dim);
    draw_text(g.board, row++, 2, "Built with Nocterm.", dim);
}

static void render_all(void){
    if(g.state==STATE_MENU){ render_menu(); return; }
    render_board();
    render_status();
}

/* ------------------------------------------------------------------ */
/* Game flow                                                          */
/* ------------------------------------------------------------------ */

static int insufficient_material(const pos_t* p){
    int minors=0, others=0;
    for(int r=0;r<8;r++)for(int c=0;c<8;c++){
        int8_t pc=p->b[r][c]; if(!pc) continue; int t=pc>0?pc:-pc;
        if(t==KING) continue;
        if(t==KNIGHT||t==BISHOP) minors++;
        else others++;
    }
    return others==0 && minors<=1; /* K vs K, or K(+ one minor) vs K */
}

static void refresh_game_status(void){
    g.legal_count = gen_legal(&g.pos, g.legal);
    g.in_check_now = in_check(&g.pos, g.pos.side);
    if(g.legal_count==0){
        g.result = g.in_check_now ? RESULT_CHECKMATE : RESULT_STALEMATE;
    } else if(g.pos.halfmove>=100 || insufficient_material(&g.pos)){
        g.result = RESULT_DRAW;
    } else {
        g.result = RESULT_NONE;
    }
}

static void apply_move(const move_t* m){
    if(g.hist_len < HIST_MAX) g.hist[g.hist_len++] = g.pos;
    char a[3],b[3]; square_name(m->fr,m->fc,a); square_name(m->tr,m->tc,b);
    sprintf(g.message, "Last: %s-%s%s", a, b, m->promo?"=Q/R/B/N":"");
    g.last_fr=m->fr; g.last_fc=m->fc; g.last_tr=m->tr; g.last_tc=m->tc;
    make_move(&g.pos, m);
    g.sel_r=g.sel_c=-1;
    g.promo_pending=0;
    refresh_game_status();
}

static void start_game(game_mode_t mode, int human_side){
    pos_startpos(&g.pos);
    g.state = STATE_PLAY;
    g.mode = mode;
    g.bot_side = (mode==MODE_VS_BOT) ? -human_side : 0;
    g.sel_r=g.sel_c=-1;
    g.cur_r=6; g.cur_c=4;           /* a sensible starting cursor (e2) */
    g.promo_pending=0;
    g.last_fr=g.last_fc=g.last_tr=g.last_tc=-1;
    g.hist_len=0;
    g.message[0]=0;
    refresh_game_status();
}

static void go_to_menu(void){
    g.state = STATE_MENU;
}

static void do_undo(void){
    if(g.hist_len==0) return;
    /* In bot mode, undo back to the human's turn (pop the bot reply too). */
    int pops = (g.mode==MODE_VS_BOT) ? 2 : 1;
    while(pops-- > 0 && g.hist_len>0){
        g.pos = g.hist[--g.hist_len];
    }
    g.sel_r=g.sel_c=-1;
    g.promo_pending=0;
    g.last_fr=g.last_fc=g.last_tr=g.last_tc=-1;
    g.message[0]=0;
    refresh_game_status();
}

/* Try to play the cursor square as a destination for the selected piece. */
static void try_move_to_cursor(void){
    /* Count promotion options at this from/to (a pawn reaching the last rank). */
    int promo_here=0;
    for(int i=0;i<g.legal_count;i++){
        move_t* m=&g.legal[i];
        if(m->fr==g.sel_r && m->fc==g.sel_c && m->tr==g.cur_r && m->tc==g.cur_c){
            if(m->promo){ promo_here=1; }
            else { apply_move(m); return; }
        }
    }
    if(promo_here){
        g.promo_pending=1;
        g.promo_from_to.fr=g.sel_r; g.promo_from_to.fc=g.sel_c;
        g.promo_from_to.tr=g.cur_r; g.promo_from_to.tc=g.cur_c;
    }
}

static void choose_promotion(int type){
    for(int i=0;i<g.legal_count;i++){
        move_t* m=&g.legal[i];
        if(m->fr==g.promo_from_to.fr && m->fc==g.promo_from_to.fc &&
           m->tr==g.promo_from_to.tr && m->tc==g.promo_from_to.tc && m->promo==type){
            apply_move(m); return;
        }
    }
}

static void select_or_move(void){
    if(g.result!=RESULT_NONE) return;
    if(g.mode==MODE_VS_BOT && g.pos.side==g.bot_side) return; /* not your turn */

    int8_t pc = g.pos.b[g.cur_r][g.cur_c];
    if(g.sel_r<0){
        /* select own piece */
        if(pc!=0 && (pc>0)==(g.pos.side>0)) { g.sel_r=g.cur_r; g.sel_c=g.cur_c; }
    } else {
        if(g.cur_r==g.sel_r && g.cur_c==g.sel_c){ g.sel_r=g.sel_c=-1; }            /* deselect */
        else if(pc!=0 && (pc>0)==(g.pos.side>0)){ g.sel_r=g.cur_r; g.sel_c=g.cur_c; } /* reselect */
        else { try_move_to_cursor(); }
    }
}

/* ------------------------------------------------------------------ */
/* Input + bot timer                                                  */
/* ------------------------------------------------------------------ */

NOCTERM_WIDGET_KEY_HANDLER(handle_key){
    nocterm_key_event_t ev = nocterm_key_translate(key);

    if(g.state==STATE_MENU){
        if(key->buffer_length==1){
            char c = key->buffer[0];
            if(c=='1'){ start_game(MODE_TWO_PLAYER, 1); render_all(); return; }
            if(c=='2'){ start_game(MODE_VS_BOT, 1);     render_all(); return; }
            if(c=='3'){ start_game(MODE_VS_BOT, -1);    render_all(); return; }
            if(c=='q'||c=='Q'){ nocterm_page_stack_pop(); return; }
        }
        if(ev==NOCTERM_KEY_EVENT_MOUSE && nocterm_mouse_translate(key).button==NOCTERM_MOUSE_BUTTON_LMB){
            /* Menu options are drawn at these board-local rows. */
            switch(click_to_menu_row(nocterm_mouse_translate(key).row)){
                case 5: start_game(MODE_TWO_PLAYER, 1); render_all(); break;
                case 6: start_game(MODE_VS_BOT, 1);     render_all(); break;
                case 7: start_game(MODE_VS_BOT, -1);    render_all(); break;
                case 8: nocterm_page_stack_pop(); break;
                default: break;
            }
        }
        return;
    }

    /* STATE_PLAY */
    if(key->buffer_length==1){
        char c = key->buffer[0];
        if(c=='q'||c=='Q'){ nocterm_page_stack_pop(); return; }
        if(c=='n'||c=='N'){ go_to_menu(); render_all(); return; }
        if(c=='u'||c=='U'){ do_undo(); render_all(); return; }
        if(g.promo_pending){
            int t=0;
            if(c=='q'||c=='Q') t=QUEEN; else if(c=='r'||c=='R') t=ROOK;
            else if(c=='b'||c=='B') t=BISHOP; else if(c=='n'||c=='N') t=KNIGHT;
            if(t){ choose_promotion(t); render_all(); }
            return;
        }
    }

    if(g.promo_pending) return; /* ignore navigation while choosing a promotion */

    switch(ev){
        case NOCTERM_KEY_EVENT_UP:    if(g.cur_r>0) g.cur_r--; break;
        case NOCTERM_KEY_EVENT_DOWN:  if(g.cur_r<7) g.cur_r++; break;
        case NOCTERM_KEY_EVENT_LEFT:  if(g.cur_c>0) g.cur_c--; break;
        case NOCTERM_KEY_EVENT_RIGHT: if(g.cur_c<7) g.cur_c++; break;
        case NOCTERM_KEY_EVENT_ENTER: select_or_move(); break;
        case NOCTERM_KEY_EVENT_PRINTABLE:
            if(key->buffer_length==1 && key->buffer[0]==' ') select_or_move();
            break;
        case NOCTERM_KEY_EVENT_MOUSE:{
            nocterm_mouse_event_t me = nocterm_mouse_translate(key);
            int r,c;
            if(me.button==NOCTERM_MOUSE_BUTTON_LMB && click_to_square(me.row, me.col, &r, &c)){
                g.cur_r=r; g.cur_c=c;          /* move cursor to the clicked square ... */
                select_or_move();              /* ... and select / move there */
            } else if(me.button==NOCTERM_MOUSE_BUTTON_RMB){
                g.sel_r=g.sel_c=-1;            /* right-click clears the current selection */
            }
        }break;
        default: break;
    }
    render_all();
}

NOCTERM_TIMER_CALLBACK(bot_tick){
    if(g.state!=STATE_PLAY || g.mode!=MODE_VS_BOT) return;
    if(g.result!=RESULT_NONE || g.promo_pending) return;
    if(g.pos.side!=g.bot_side) return;

    move_t best;
    if(engine_best_move(&g.pos, &best)){
        apply_move(&best);
        render_all();
    }
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */

int main(void){
    setlocale(LC_ALL, "");

    g.board  = nocterm_widget_new(11, 30, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);
    g.status = nocterm_widget_new(16, 36, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);

    nocterm_widget_t* container = nocterm_widget_new(17, 70, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_add_subwidget(container, g.board);
    nocterm_widget_add_subwidget(container, g.status);
    nocterm_widget_set_position(g.board, 1, 1);
    nocterm_widget_set_position(g.status, 1, 33);

    /* Route clicks on the board cells to the container's key handler: the mouse
     * controller delivers a click to the clicked widget's `owner`, and only when
     * that owner is focused or the widget has click activation enabled. */
    g.board->owner = container;
    nocterm_widget_set_click_activation(g.board, true);

    nocterm_widget_set_key_handler(container, handle_key);

    nocterm_page_t* page = nocterm_page_new("Chess", sizeof("Chess"), container);
    nocterm_page_stack_push(page);

    go_to_menu();
    render_all();

    nocterm_timer_t* timer = nocterm_timer_create(container, 250, bot_tick, NULL);
    nocterm_timer_start(timer);

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    nocterm_timer_delete(timer);
    nocterm_page_delete(page);
    nocterm_widget_delete(container);
    nocterm_widget_delete(g.board);
    nocterm_widget_delete(g.status);

    return 0;
}
