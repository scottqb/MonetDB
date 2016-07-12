/*
 * The contents of this file are subject to the MonetDB Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.monetdb.org/Legal/MonetDBLicense
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the MonetDB Database System.
 *
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-July 2008 CWI.
 *                * Copyright August 2008-2014 MonetDB B.V.
 * All Rights Reserved.
 */

/*
 * (c)2014 author Martin Kersten
 */

#ifndef _MOSAIC_DICT_
#define _MOSAIC_DICT_

#include <mal.h>
#include "mal_interpreter.h"
#include "mal_client.h"

#define DICTTHRESHOLD 4192
#define DICTSIZE 256 

mal_export void MOScreatedictionary(Client cntxt, MOStask task);
mal_export void MOSdump_dictionary(Client cntxt, MOStask task);
mal_export void MOSlayout_dictionary(Client cntxt, MOStask task, BAT *btech, BAT *bcount, BAT *binput, BAT *boutput, BAT *bproperties);
mal_export void MOSlayout_dictionary_hdr(Client cntxt, MOStask task, BAT *btech, BAT *bcount, BAT *binput, BAT *boutput, BAT *bproperties);
mal_export void MOSadvance_dictionary(Client cntxt, MOStask task);
mal_export void MOSskip_dictionary(Client cntxt, MOStask task);
mal_export flt  MOSestimate_dictionary(Client cntxt, MOStask task);
mal_export void MOScompress_dictionary(Client cntxt, MOStask task);
mal_export void MOSdecompress_dictionary(Client cntxt, MOStask task);
mal_export str MOSsubselect_dictionary(Client cntxt,  MOStask task, void *low, void *hgh, bit *li, bit *hi, bit *anti);
mal_export str MOSthetasubselect_dictionary(Client cntxt,  MOStask task, void *val, str oper);
mal_export str MOSprojection_dictionary(Client cntxt,  MOStask task);
mal_export str MOSsubjoin_dictionary(Client cntxt,  MOStask task);
#endif /* _MOSAIC_DICT_ */
