/* What this application does:
This application has an interface where you choose files(.txt) in the current working directory
and store the names of those files in a seperate table. Then, through this table, you can view
the chosen file names.. And through our application, users can choose to view the contents of
one of these files through an input.
What it should contain:
Create a Command line Interface application that has a main menu consisting of three options:
Options:
1. Create a database and table. ( Use any database that you like - SQLite should be ideal -
easier setup )
	Table metadata: Id, file path, file size.
2. Get files in the current directory.
	After listing the files in the current directory in an ordered list fashion numbered.
	1. Enter a number from ( 0 to n ( no of files present in the current directory ) to be
	added to the table ( that you had created in the first option ). If a file has been
	added already to the table, please don’t list it.
	2. Enter -1 to go back to the main menu.
3. Get table info.
	This should print the table contents in a viewable manner.
	1. Enter a number from 0 to no of table rows in the table.
	Corresponding to the number chosen, the file contents are displayed in the CLI.
	2. Enter -1 to go back to the main menu.
4. Delete table contents.
5. Exit. */
//todo: make file size as long

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <sqlite3.h>
#include <dirent.h>
#include <stdbool.h>
#include <libgen.h>

#define MAX_PATH 150
#define MAX_FILE 150

const char *homedir;
char pwd[MAX_PATH];
char **db_files;
char *table_name;
int num_db_files = 0;

char* delete_char(char *str, char c) {
	static char str2[MAX_PATH];
	size_t i, j = 0;
	for (i = 0; i < strlen(str); i++) {
		if (str[i] != c) {
			str2[j++] = str[i];
		}
		else
			continue;	 
	}	
	return str2;	
}

int get_max(void *max, int argc, char **argv, char **azColName) {
	int i;
	int *max_id = max;
	for (i = 0; i < argc; i++) {
		*max_id = atoi(argv[1]);
	}
	return *max_id;
}

bool is_file_in_db(char *fname) {
	int i;
	for (i = 0; i < num_db_files; i++) {
		if (!strcmp(fname, db_files[i])) {
			return true;
		}
	}
	return false;
}

int get_id(sqlite3 *db) {
	int max_id = 0;
	if (num_db_files != 0) {
		sqlite3_stmt *res;
		char *err_msg = 0;
		char *sql1 = "select max(id) from ";
		char *sql = (char*)calloc(strlen(sql1) + strlen(table_name) + 1, sizeof(char));
		strcat(sql, sql1);
		strcat(sql, table_name);
		int rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "sql err: %s\n", sqlite3_errmsg(db));
			sqlite3_finalize(res);
			sqlite3_close(db);
			exit(1);
		}
		
		int step = sqlite3_step(res);

		if (step == SQLITE_ROW) {
			max_id = sqlite3_column_int(res, 0);
		}

		/*int rc = sqlite3_exec(db, sql, get_max, &max_id, &err_msg);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "sql err: %s\n", err_msg);
			sqlite3_free(err_msg);
			sqlite3_close(db);
			exit(1);
		}*/
		free(sql);
	}
	return max_id + 1;
}

size_t get_size(char *fname) {
	FILE *fp = fopen(fname, "r");
	if (fp == NULL) {
		printf("%s not found\n", fname);
		exit(1);
	}
	fseek(fp, 0L, SEEK_END);
	size_t res = ftell(fp);
	fclose(fp);
	return res;
}

void insert_file(sqlite3 *db, char *fname) {
	if (!is_file_in_db(fname)) {
		db_files[num_db_files] = (char*)calloc(MAX_FILE, sizeof(char));
		strcpy(db_files[num_db_files++], fname);
		int fid = get_id(db);
		char *fpath = (char*)calloc(strlen(pwd) + strlen(fname) + 2, sizeof(char));
		strcat(fpath, pwd);
		strcat(fpath, "/");
		strcat(fpath, fname);

		size_t fsize = get_size(fname);
		char *sql1 = "insert into ";
		char *sql2 = "(id, path, size) values(?,?,?)";
		char *sql;
		sql = (char*)calloc(strlen(sql1) + strlen(table_name) + strlen(sql2) + 1, sizeof(char));
		strcat(sql, sql1);
		strcat(sql, table_name);
		strcat(sql, sql2);

		sqlite3_stmt *smt;
		int rc = sqlite3_prepare_v2(db, sql, -1, &smt, 0);
		if (rc == SQLITE_OK) {
			sqlite3_bind_int(smt, 1, fid);
			sqlite3_bind_text(smt, 2, fpath, -1, 0);
			sqlite3_bind_int(smt, 3, fsize);
		}
		else {
			fprintf(stderr, "cannot prepare stmt: %s\n", sqlite3_errmsg(db));
			exit(1);
		}

		rc = sqlite3_step(smt);
		if (rc != SQLITE_DONE) {
			fprintf(stderr, "execution failed: %s", sqlite3_errmsg(db));
		}
		sqlite3_finalize(smt);
		free(sql);
		free(fpath);
	}
	else {
		printf("file %s already added\n", fname);
	}
}

void delete_element_from_arr(char *fname) {
	int i;
	for (i = 0; i < num_db_files; i++) {
		if (!strcmp(fname, db_files[i])) {
			break;
		}
	}
	num_db_files--;
	if (i == num_db_files) {
		db_files[i][0] = '\0';
	}
	else {
		while (i < num_db_files) {
			db_files[i][0] = '\0';
			strcpy(db_files[i], db_files[++i]);
		}
	}
	
}

void delete_file(sqlite3 *db, char *fname) {
	if (is_file_in_db(fname)) {

		char *fpath = (char*)calloc(strlen(pwd) + strlen(fname) + 2, sizeof(char));
		strcat(fpath, pwd);
		strcat(fpath, "/");
		strcat(fpath,fname);
		char *err_msg = 0;
		
		char *sql1 = "delete from ";
		char *sql2 = " where path like '%";
		char *sql3 = "%'";
		char *sql = (char*)calloc(strlen(sql1) + strlen(table_name) + strlen(sql2) + strlen(sql3) + strlen(fpath) + 1, sizeof(char));
		strcat(sql, sql1);
		strcat(sql, table_name);
		strcat(sql, sql2);
		strcat(sql, fpath);
		strcat(sql, sql3);

		int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "sql err: %s\n", err_msg);
			sqlite3_free(err_msg);
			sqlite3_close(db);
			exit(1);
		}

		printf("%s\n", sql);
		free(sql);
		free(fpath);
		printf("file %s deleted\n", fname);
		delete_element_from_arr(fname);
	}
	else {
		printf("file %s not in db\n", fname);
	}
}

void create_table(sqlite3 *db) {
	char *err_msg = 0;
	char *sql1 = "create table if not exists ";
	char *sql2 = "(id integer primary key, path text not null, size integer not null);";
	char *sql = (char*)calloc(strlen(sql1) + strlen(sql2) + strlen(table_name) + 3, sizeof(char));
	strcat(sql, sql1);
	strcat(sql, table_name);
	strcat(sql, sql2);
	int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "sql err: %s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		exit(1);
	}
	free(sql);
	printf("table %s created\n", table_name);
}

void list_files(sqlite3 *db, char **list) {
	DIR *d;
	struct dirent *dir;
	d = opendir(".");
	int i = 0;
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (strcmp(".", dir->d_name) && strcmp("..", dir->d_name)) {
				if (!is_file_in_db(dir->d_name)) {
					printf("%d. %s\n", i, dir->d_name);
					list[i] = (char*)calloc(MAX_FILE, sizeof(char));
					strcpy(list[i++], dir->d_name);
				}
			}
		}
		closedir(d);
	}
}

int num_files() {
	DIR *d;
	struct dirent *dir;
	d = opendir(".");
	int num = 0;
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (strcmp(".", dir->d_name) && strcmp("..", dir->d_name)) {
				num++;
			}
		}
	}
	return num;
}

int append_db_files(void *unused, int argc, char **argv, char **acColName) {
	unused = 0;
	int i;
	for (i = 0; i < argc; i++) {
		//strcpy(db_files[num_db_files++], argv[i]);
		db_files[num_db_files] = (char*)calloc(MAX_FILE, sizeof(char));
		db_files[num_db_files++] = basename(argv[i]);
	}
	return 0;
}

void init_db_files(sqlite3 *db) {
	int i;
	char *err_msg = 0;
	char *sql1 = "select path from ";
	char *sql = (char*)calloc(strlen(sql1) + strlen(table_name) + 1, sizeof(char));
	strcat(sql, sql1);
	strcat(sql, table_name);
	int rc = sqlite3_exec(db, sql, append_db_files, 0, &err_msg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "sql err: %s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		exit(1);
	}
	free(sql);
}

int print_table(void *unused, int argc, char **argv, char **col) {
	unused = 0;
	int i;
	for (i = 0; i < argc; i++) {
		printf("\t%s", argv[i]);
	}
	printf("\n");
	return 0;
}

void table_info(sqlite3 *db) {
	char *err_msg;
	char *sql1 = "select * from ";
	char *sql = (char*)calloc(strlen(sql1) + strlen(table_name), sizeof(char));
	strcat(sql, sql1);
	strcat(sql, table_name);
	printf("\tid\tpath\tsize\n");
	int rc = sqlite3_exec(db, sql, print_table, 0, &err_msg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "sql error: %s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		exit(1);
	}
	free(sql);
}

void print_file(char *fpath) {
	FILE *f;
	f = fopen(fpath, "r");
	if (f == NULL) {
		fprintf(stderr, "cant open file\n");
		exit(1);
	}
	char c = fgetc(f);
	while (c != EOF) {
		printf("%c", c);
		c = fgetc(f);
	}
	fclose(f);
}

bool table_exists(sqlite3 *db) {
	//select name from sqlite_master where type='table' and name=tname
	char *err_msg = 0;
	char *sql = "select name from sqlite_master where type='table' and name = '?'";
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "sql err: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(res);
		sqlite3_close(db);
		exit(1);
	}

	int step = sqlite3_step(res);
	if (step == SQLITE_ROW) {
		return (sqlite3_column_type(res, 0) != SQLITE_NULL);
	}
	return false;
}

int main() {
	if ((homedir = getenv("HOME")) == NULL) {
		homedir = getpwuid(getuid())->pw_dir;
	}
	getcwd(pwd, sizeof(pwd));
	table_name = delete_char(pwd, '/');
	int i;
	char *dbpath = (char*)calloc(strlen(homedir) + 12, sizeof(char));
	strcat(dbpath, homedir);
	strcat(dbpath, "/");
	strcat(dbpath, ".cfiles.db");

	db_files = (char**)calloc(num_files(), sizeof(char*));

	sqlite3 *db;
	int rc;
	char *err_msg = 0;
	//bool db_exists = access(dbpath, F_OK) == 0;

	/*if (db_exists) {
		rc = sqlite3_open(dbpath, &db);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "cannot open db: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
			return 1;
		}
	}*/

	rc = sqlite3_open(dbpath, &db);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "cannot open db: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}

	char *menu = "1. Create table and/or db.\n2. Get files in current directory.\n3. Get table info.\n4. Delete table contents.\n5. Exit.";

	while (true) {
		printf("%s\n", menu);
		printf("your option: ");
		int option;
		scanf("%d", &option);

		switch (option) {
			case 1:
				create_table(db);
				init_db_files(db);
				break;
			case 2:
				char **list = (char**)calloc(num_files(), sizeof(char*));
				list_files(db, list);
				printf("your option: ");
				int foption;
				scanf("%d", &foption);
				if (foption == -1) {
					continue;
				}
				else {
					insert_file(db, list[foption]);
				}
				break;
			case 3:
				table_info(db);
				int tchoice;
				printf("\nyour option: ");
				scanf("%d", &tchoice);
				if (tchoice == -1) {
					continue;
				}
				else {
					print_file(db_files[tchoice]);
				}
				break;
			case 4:
				table_info(db);
				int dchoice;
				printf("\nyour option: ");
				scanf("%d", &dchoice);
				if (dchoice == -1) {
					continue;
				}
				else {
					delete_file(db, db_files[dchoice]);
				}
				break;
			default:
				exit(0);
		}
	}

	free(db_files);
	sqlite3_close(db);
	return 0;
}
