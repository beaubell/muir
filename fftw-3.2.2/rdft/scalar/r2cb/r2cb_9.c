/*
 * Copyright (c) 2003, 2007-8 Matteo Frigo
 * Copyright (c) 2003, 2007-8 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* This file was automatically generated --- DO NOT EDIT */
/* Generated on Sun Jul 12 06:45:42 EDT 2009 */

#include "codelet-rdft.h"

#ifdef HAVE_FMA

/* Generated by: ../../../genfft/gen_r2cb -fma -reorder-insns -schedule-for-pipeline -compact -variables 4 -pipeline-latency 4 -sign 1 -n 9 -name r2cb_9 -include r2cb.h */

/*
 * This function contains 32 FP additions, 24 FP multiplications,
 * (or, 8 additions, 0 multiplications, 24 fused multiply/add),
 * 40 stack variables, 12 constants, and 18 memory accesses
 */
#include "r2cb.h"

static void r2cb_9(R *R0, R *R1, R *Cr, R *Ci, stride rs, stride csr, stride csi, INT v, INT ivs, INT ovs)
{
     DK(KP1_326827896, +1.326827896337876792410842639271782594433726619);
     DK(KP1_705737063, +1.705737063904886419256501927880148143872040591);
     DK(KP766044443, +0.766044443118978035202392650555416673935832457);
     DK(KP1_532088886, +1.532088886237956070404785301110833347871664914);
     DK(KP984807753, +0.984807753012208059366743024589523013670643252);
     DK(KP1_969615506, +1.969615506024416118733486049179046027341286503);
     DK(KP839099631, +0.839099631177280011763127298123181364687434283);
     DK(KP176326980, +0.176326980708464973471090386868618986121633062);
     DK(KP866025403, +0.866025403784438646763723170752936183471402627);
     DK(KP500000000, +0.500000000000000000000000000000000000000000000);
     DK(KP1_732050807, +1.732050807568877293527446341505872366942805254);
     DK(KP2_000000000, +2.000000000000000000000000000000000000000000000);
     INT i;
     for (i = v; i > 0; i = i - 1, R0 = R0 + ovs, R1 = R1 + ovs, Cr = Cr + ivs, Ci = Ci + ivs, MAKE_VOLATILE_STRIDE(rs), MAKE_VOLATILE_STRIDE(csr), MAKE_VOLATILE_STRIDE(csi)) {
	  E T4, Th, T3, Tb, Tp, Tk, T7, Tf, Ti, Ta, T1, T2;
	  Ta = Ci[WS(csi, 3)];
	  T1 = Cr[0];
	  T2 = Cr[WS(csr, 3)];
	  T4 = Cr[WS(csr, 1)];
	  Th = Ci[WS(csi, 1)];
	  {
	       E T5, T9, T6, Td, Te;
	       T5 = Cr[WS(csr, 4)];
	       T9 = T1 - T2;
	       T3 = FMA(KP2_000000000, T2, T1);
	       T6 = Cr[WS(csr, 2)];
	       Td = Ci[WS(csi, 4)];
	       Te = Ci[WS(csi, 2)];
	       Tb = FNMS(KP1_732050807, Ta, T9);
	       Tp = FMA(KP1_732050807, Ta, T9);
	       Tk = T6 - T5;
	       T7 = T5 + T6;
	       Tf = Td + Te;
	       Ti = Td - Te;
	  }
	  {
	       E Tu, To, Tt, Tn, Tc, T8;
	       Tc = FNMS(KP500000000, T7, T4);
	       T8 = T4 + T7;
	       {
		    E Tw, Tj, Tr, Tg, Tv;
		    Tw = Ti + Th;
		    Tj = FNMS(KP500000000, Ti, Th);
		    Tr = FMA(KP866025403, Tf, Tc);
		    Tg = FNMS(KP866025403, Tf, Tc);
		    Tv = T3 - T8;
		    R0[0] = FMA(KP2_000000000, T8, T3);
		    {
			 E Tq, Tl, Ts, Tm;
			 Tq = FMA(KP866025403, Tk, Tj);
			 Tl = FNMS(KP866025403, Tk, Tj);
			 R0[WS(rs, 3)] = FMA(KP1_732050807, Tw, Tv);
			 R1[WS(rs, 1)] = FNMS(KP1_732050807, Tw, Tv);
			 Ts = FNMS(KP176326980, Tr, Tq);
			 Tu = FMA(KP176326980, Tq, Tr);
			 Tm = FNMS(KP839099631, Tl, Tg);
			 To = FMA(KP839099631, Tg, Tl);
			 R0[WS(rs, 1)] = FNMS(KP1_969615506, Ts, Tp);
			 Tt = FMA(KP984807753, Ts, Tp);
			 R1[0] = FMA(KP1_532088886, Tm, Tb);
			 Tn = FNMS(KP766044443, Tm, Tb);
		    }
	       }
	       R1[WS(rs, 2)] = FNMS(KP1_705737063, Tu, Tt);
	       R0[WS(rs, 4)] = FMA(KP1_705737063, Tu, Tt);
	       R0[WS(rs, 2)] = FNMS(KP1_326827896, To, Tn);
	       R1[WS(rs, 3)] = FMA(KP1_326827896, To, Tn);
	  }
     }
}

static const kr2c_desc desc = { 9, "r2cb_9", {8, 0, 24, 0}, &GENUS };

void X(codelet_r2cb_9) (planner *p) {
     X(kr2c_register) (p, r2cb_9, &desc);
}

#else				/* HAVE_FMA */

/* Generated by: ../../../genfft/gen_r2cb -compact -variables 4 -pipeline-latency 4 -sign 1 -n 9 -name r2cb_9 -include r2cb.h */

/*
 * This function contains 32 FP additions, 18 FP multiplications,
 * (or, 22 additions, 8 multiplications, 10 fused multiply/add),
 * 35 stack variables, 12 constants, and 18 memory accesses
 */
#include "r2cb.h"

static void r2cb_9(R *R0, R *R1, R *Cr, R *Ci, stride rs, stride csr, stride csi, INT v, INT ivs, INT ovs)
{
     DK(KP984807753, +0.984807753012208059366743024589523013670643252);
     DK(KP173648177, +0.173648177666930348851716626769314796000375677);
     DK(KP300767466, +0.300767466360870593278543795225003852144476517);
     DK(KP1_705737063, +1.705737063904886419256501927880148143872040591);
     DK(KP642787609, +0.642787609686539326322643409907263432907559884);
     DK(KP766044443, +0.766044443118978035202392650555416673935832457);
     DK(KP1_326827896, +1.326827896337876792410842639271782594433726619);
     DK(KP1_113340798, +1.113340798452838732905825904094046265936583811);
     DK(KP500000000, +0.500000000000000000000000000000000000000000000);
     DK(KP866025403, +0.866025403784438646763723170752936183471402627);
     DK(KP2_000000000, +2.000000000000000000000000000000000000000000000);
     DK(KP1_732050807, +1.732050807568877293527446341505872366942805254);
     INT i;
     for (i = v; i > 0; i = i - 1, R0 = R0 + ovs, R1 = R1 + ovs, Cr = Cr + ivs, Ci = Ci + ivs, MAKE_VOLATILE_STRIDE(rs), MAKE_VOLATILE_STRIDE(csr), MAKE_VOLATILE_STRIDE(csi)) {
	  E T3, Tq, Tc, Tk, Tj, T8, Tm, Ts, Th, Tr, Tw, Tx;
	  {
	       E Tb, T1, T2, T9, Ta;
	       Ta = Ci[WS(csi, 3)];
	       Tb = KP1_732050807 * Ta;
	       T1 = Cr[0];
	       T2 = Cr[WS(csr, 3)];
	       T9 = T1 - T2;
	       T3 = FMA(KP2_000000000, T2, T1);
	       Tq = T9 + Tb;
	       Tc = T9 - Tb;
	  }
	  {
	       E T4, T7, Ti, Tg, Tl, Td;
	       T4 = Cr[WS(csr, 1)];
	       Tk = Ci[WS(csi, 1)];
	       {
		    E T5, T6, Te, Tf;
		    T5 = Cr[WS(csr, 4)];
		    T6 = Cr[WS(csr, 2)];
		    T7 = T5 + T6;
		    Ti = KP866025403 * (T5 - T6);
		    Te = Ci[WS(csi, 4)];
		    Tf = Ci[WS(csi, 2)];
		    Tg = KP866025403 * (Te + Tf);
		    Tj = Tf - Te;
	       }
	       T8 = T4 + T7;
	       Tl = FMA(KP500000000, Tj, Tk);
	       Tm = Ti + Tl;
	       Ts = Tl - Ti;
	       Td = FNMS(KP500000000, T7, T4);
	       Th = Td - Tg;
	       Tr = Td + Tg;
	  }
	  R0[0] = FMA(KP2_000000000, T8, T3);
	  Tw = T3 - T8;
	  Tx = KP1_732050807 * (Tk - Tj);
	  R1[WS(rs, 1)] = Tw - Tx;
	  R0[WS(rs, 3)] = Tw + Tx;
	  {
	       E Tp, Tn, To, Tv, Tt, Tu;
	       Tp = FMA(KP1_113340798, Th, KP1_326827896 * Tm);
	       Tn = FNMS(KP642787609, Tm, KP766044443 * Th);
	       To = Tc - Tn;
	       R1[0] = FMA(KP2_000000000, Tn, Tc);
	       R1[WS(rs, 3)] = To + Tp;
	       R0[WS(rs, 2)] = To - Tp;
	       Tv = FMA(KP1_705737063, Tr, KP300767466 * Ts);
	       Tt = FNMS(KP984807753, Ts, KP173648177 * Tr);
	       Tu = Tq - Tt;
	       R0[WS(rs, 1)] = FMA(KP2_000000000, Tt, Tq);
	       R0[WS(rs, 4)] = Tu + Tv;
	       R1[WS(rs, 2)] = Tu - Tv;
	  }
     }
}

static const kr2c_desc desc = { 9, "r2cb_9", {22, 8, 10, 0}, &GENUS };

void X(codelet_r2cb_9) (planner *p) {
     X(kr2c_register) (p, r2cb_9, &desc);
}

#endif				/* HAVE_FMA */
