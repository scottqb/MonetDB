/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2017 MonetDB B.V.
 */

/*
 * Selectively inject serialization operations when we know the
 * raw footprint of the query exceeds 80% of RAM.
 */

#include "monetdb_config.h"
#include "mal_instruction.h"
#include "opt_volcano.h"

// delaying the startup should not be continued throughout the plan
// after the startup phase there should be intermediate work to do
//A heuristic to check it
#define MAXdelays 128

str
OPTvolcanoImplementation(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	int i, limit;
	int mvcvar = -1;
	int count=0;
	InstrPtr p,q, *old = mb->stmt;
	char buf[256];
	lng usec = GDKusec();

	(void) pci;
	(void) cntxt;
	(void) stk;		/* to fool compilers */

    if ( mb->inlineProp )
        return MAL_SUCCEED;

    limit= mb->stop;
    if ( newMalBlkStmt(mb, mb->ssize + 20) < 0)
		throw(MAL,"optimizer.volcano",MAL_MALLOC_FAIL);

	for (i = 0; i < limit; i++) {
		p = old[i];

		pushInstruction(mb,p);
		if( getModuleId(p) == sqlRef && getFunctionId(p)== mvcRef ){
			mvcvar = getArg(p,0);
			continue;
		}

		if( count < MAXdelays && getModuleId(p) == algebraRef ){
			if( getFunctionId(p) == selectRef ||
				getFunctionId(p) == thetaselectRef ||
				getFunctionId(p) == likeselectRef ||
				getFunctionId(p) == joinRef
			){
				q= newInstruction(0,languageRef,blockRef);
				setDestVar(q, newTmpVariable(mb,TYPE_any));
				q =  pushArgument(mb,q,mvcvar);
				q =  pushArgument(mb,q,getArg(p,0));
				mvcvar=  getArg(q,0);
				pushInstruction(mb,q);
				count++;
			}
			continue;
		}
		if( count < MAXdelays && getModuleId(p) == groupRef ){
			if( getFunctionId(p) == subgroupdoneRef || getFunctionId(p) == groupdoneRef ){
				q= newInstruction(0,languageRef,blockRef);
				setDestVar(q, newTmpVariable(mb,TYPE_any));
				q =  pushArgument(mb,q,mvcvar);
				q =  pushArgument(mb,q,getArg(p,0));
				mvcvar=  getArg(q,0);
				pushInstruction(mb,q);
				count++;
			}
		}
		if( getModuleId(p) == sqlRef){
			if ( getFunctionId(p) == bindRef ||
				getFunctionId(p) == bindidxRef || 
				getFunctionId(p)== tidRef ||
				getFunctionId(p)== appendRef ||
				getFunctionId(p)== updateRef ||
				getFunctionId(p)== deleteRef
			){
				setArg(p,p->retc,mvcvar);
			}
		}
	} 
	GDKfree(old);

    /* Defense line against incorrect plans */
    if( count){
        chkTypes(cntxt->fdout, cntxt->nspace, mb, FALSE);
        chkFlow(cntxt->fdout, mb);
        chkDeclarations(cntxt->fdout, mb);
    }
    /* keep all actions taken as a post block comment */
	usec = GDKusec()- usec;
    snprintf(buf,256,"%-20s actions=%2d time=" LLFMT " usec","volcano",count,usec);
    newComment(mb,buf);
	if( count >= 0)
		addtoMalBlkHistory(mb);

	return MAL_SUCCEED;
}
