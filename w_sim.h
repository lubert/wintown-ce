/* w_sim.h - Force-include for s_sim.c compilation only
 * Renames conflicting functions that WinTown reimplements
 */
#define _W_SIM_H

#define SimFrame _orig_SimFrame
#define Simulate _orig_Simulate
#define DoSimInit _orig_DoSimInit
#define MapScan _orig_MapScan
#define GetBoatDis _orig_GetBoatDis
#define DoMeltdown _orig_DoMeltdown
#define Rand _orig_Rand
#define Rand16 _orig_Rand16
#define Rand16Signed _orig_Rand16Signed
#define RandomlySeedRand _orig_RandomlySeedRand
#define SeedRand _orig_SeedRand

extern unsigned short SimRand16U(void);
#define sim_rand() ((int)SimRand16U())
#define sim_srand(s) srand((unsigned int)(s))

#define doAllGraphs()

#ifdef _WIN32_WCE
struct timeval {
    long tv_sec;
    long tv_usec;
};
#endif

static short NewMapFlags[16];
static short NewMap;
static short NewGraph;
