#include "media_db.h"
#include "media_context.h"

agc_status_t media_db_load_lbo()
{	
	int  iptype;
	const char *addr = NULL;
	agc_std_sockaddr_t servaddr;
	socklen_t addrlen = 0;

	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;

	int error = 0;

	if (!(db = agc_db_open_file(DATA_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_db_load_lbo open db [fail].\n");
		return AGC_STATUS_FALSE;
	}

	if (agc_db_prepare(db, "SELECT * FROM media_lbo_table;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_db_load_lbo failed .\n");
		agc_db_close(db);
		return AGC_STATUS_FALSE;
	}	

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
			{
				columns = agc_db_column_count(stmt);
				
				iptype = agc_db_column_int(stmt, 1);
				addr = (const char *)agc_db_column_text(stmt, 2);
				agc_log_printf(AGC_LOG, AGC_LOG_INFO, "media_db_load_lbo %d addr=%s.\n", iptype, addr);

				if (iptype == 0)
				{
					struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&servaddr;
					local_addr1_v4->sin_family = AF_INET;
					local_addr1_v4->sin_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
					inet_pton(AF_INET, addr, &(local_addr1_v4->sin_addr));
					addrlen = sizeof(struct sockaddr_in);
				}
				else
				{
					struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&servaddr;
					local_addr1_v6->sin6_family = AF_INET6;
					local_addr1_v6->sin6_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
					inet_pton(AF_INET6, addr, &(local_addr1_v6->sin6_addr));
					addrlen = sizeof(struct sockaddr_in6);
				}
				
				media_add_lbo_node(&servaddr, addrlen);
				break;
			}
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_db_load_lbo [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "media_db_load_lbo [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

    return AGC_STATUS_SUCCESS;
}