/*
 * Copyright (C) 1994-2018 Altair Engineering, Inc.
 * For more information, contact Altair at www.altair.com.
 *
 * This file is part of the PBS Professional ("PBS Pro") software.
 *
 * Open Source License Information:
 *
 * PBS Pro is free software. You can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * PBS Pro is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Commercial License Information:
 *
 * For a copy of the commercial license terms and conditions,
 * go to: (http://www.pbspro.com/UserArea/agreement.html)
 * or contact the Altair Legal Department.
 *
 * Altair’s dual-license business model allows companies, individuals, and
 * organizations to create proprietary derivative works of PBS Pro and
 * distribute them - whether embedded or bundled with other software -
 * under a commercial license agreement.
 *
 * Use of Altair’s trademarks, including but not limited to "PBS™",
 * "PBS Professional®", and "PBS Pro™" and Altair’s logos is subject to Altair's
 * trademark licensing policies.
 *
 */
/**
 * @file	qmove.c
 * @brief
 *  qmove - (PBS) move batch job
 *
 * @author	Terry Heidelberg
 * 			Livermore Computing
 *
 * @author	Bruce Kelly
 * 			National Energy Research Supercomputer Center
 *
 * @author	Lawrence Livermore National Laboratory
 * 			University of California
 */

#include "cmds.h"
#include "pbs_ifl.h"
#include <pbs_config.h>   /* the master config generated by configure */
#include <pbs_version.h>


int
main(int argc, char **argv, char **envp) /* qmove */
{
	int any_failed=0;

	char job_id[PBS_MAXCLTJOBID];		/* from the command line */
	char destination[PBS_MAXSERVERNAME];	/* from the command line */
	char *q_n_out, *s_n_out;

	char job_id_out[PBS_MAXCLTJOBID];
	char server_out[MAXSERVERNAME];
	char rmt_server[MAXSERVERNAME];

	/*test for real deal or just version and exit*/

	PRINT_VERSION_AND_EXIT(argc, argv);

#ifdef WIN32
	if (winsock_init()) {
		return 1;
	}
#endif

	if (argc < 3) {
		static char usage[]="usage: qmove destination job_identifier...\n";
		static char usag2[]="       qmove --version\n";
		fprintf(stderr, "%s", usage);
		fprintf(stderr, "%s", usag2);
		exit(2);
	}

	strcpy(destination, argv[1]);
	if (parse_destination_id(destination, &q_n_out, &s_n_out)) {
		fprintf(stderr, "qmove: illegally formed destination: %s\n", destination);
		exit(2);
	}

	/*perform needed security library initializations (including none)*/

	if (CS_client_init() != CS_SUCCESS) {
		fprintf(stderr, "qmove: unable to initialize security library.\n");
		exit(2);
	}

	for (optind = 2; optind < argc; optind++) {
		int connect;
		int stat=0;
		int located = FALSE;

		strcpy(job_id, argv[optind]);
		if (get_server(job_id, job_id_out, server_out)) {
			fprintf(stderr, "qmove: illegally formed job identifier: %s\n", job_id);
			any_failed = 1;
			continue;
		}
cnt:
		connect = cnt2server(server_out);
		if (connect <= 0) {
			fprintf(stderr, "qmove: cannot connect to server %s (errno=%d)\n",
				pbs_server, pbs_errno);
			any_failed = pbs_errno;
			continue;
		}

		stat = pbs_movejob(connect, job_id_out, destination, NULL);
		if (stat && (pbs_errno != PBSE_UNKJOBID)) {
			if (stat != PBSE_NEEDQUET) {
				prt_job_err("qmove", connect, job_id_out);
				any_failed = pbs_errno;
			} else {
				fprintf(stderr, "qmove: Queue type not set for queue \'%s\'\n", destination);
			}
		} else if (stat && (pbs_errno == PBSE_UNKJOBID) && !located) {
			located = TRUE;
			if (locate_job(job_id_out, server_out, rmt_server)) {
				pbs_disconnect(connect);
				strcpy(server_out, rmt_server);
				goto cnt;
			}
			prt_job_err("qmove", connect, job_id_out);
			any_failed = pbs_errno;
		}

		pbs_disconnect(connect);
	}

	/*cleanup security library initializations before exiting*/
	CS_close_app();

	exit(any_failed);
}
