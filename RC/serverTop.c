#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <ctype.h>

#define RED "\e[1;31m"
#define GREEN "\e[1;32m"
#define YELLOW "\e[1;33m"
#define BLUE "\e[1;34m"
#define MAGENTA "\e[1;35m"
#define CYAN "\e[1;36m"

#define URED "\e[4;31m"
#define UGREEN "\e[4;32m"
#define UYELLOW "\e[4;33m"
#define UBLUE "\e[4;34m"
#define UMAGENTA "\e[4;35m"
#define UCYAN "\e[4;36m"

#define RESET "\x1b[0m"
#define PORT 7777
#define TRUE 1

extern int errno;
int callback1(void *result, int argc, char **argv, char **azColName)
{

	char *query = result;

	for (int i = 0; i < argc; i++)
	{
		strcat(query, argv[i]);
		strcat(query, " ");
	}

	return 0;
}
int callback2(void *result, int argc, char **argv, char **azColName)
{

	char *query = (char *)result;

	int v[2] = {25, 7};
	for (int i = 0; i < argc; i++)
	{
		strcat(query, argv[i]);
		int n = strlen(argv[i]);
		for (int j = 0; j < v[i] - n; j++)
			strcat(query, " ");
	}
	strcat(query, "\n");
	return 0;
}
int callback3(void *result, int argc, char **argv, char **azColName)
{

	char *query = result;

	for (int i = 0; i < argc; i++)
	{
		strcat(query, argv[i]);
		strcat(query, "\n");
	}

	return 0;
}
int callback4(void *result, int argc, char **argv, char **azColName)
{

	char *query = (char *)result;

	for (int i = 0; i < argc; i++)
	{
		if (!i)
		{
			argv[i][0] = toupper(argv[i][0]);
			for (int j = 1; j < strlen(argv[i]); j++)
				argv[i][j] = '*';
		}
		strcat(query, argv[i]);
		if (!i)
			strcat(query, " : ");
	}
	strcat(query, "\n");
	return 0;
}
int ret_cod(char *s)
{
	if (!strcmp(s, "quit"))
		return -1;
	if (strstr(s, "login") == s)
		return 1;
	if (strstr(s, "register") == s)
		return 2;
	if (strstr(s, "add") == s)
		return 3;
	if (strstr(s, "delete") == s)
		return 4;
	if (strstr(s, "vote") == s)
		return 5;
	if (!strcmp(s, "list users"))
		return 6;
	if (strstr(s, "restrict") == s)
		return 7;
	if (!strcmp(s, "list admins"))
		return 8;
	if (!strcmp(s, "help"))
		return 9;
	if (strstr(s, "top") == s)
		return 10;
	if (!strcmp(s, "logout"))
		return 11;
	if (strstr(s, "link") == s)
		return 12;
	if (!strcmp(s, "genres"))
		return 13;
	if (strstr(s, "permit") == s)
		return 14;
	if (strstr(s, "comment") == s)
		return 15;
	if (strstr(s, "comm list") == s)
		return 16;
	return 0;
}

int main()
{
	struct sockaddr_in server;
	struct sockaddr_in from;
	char msg_sent[4000], msg_received[300], welcome[200] = YELLOW " ♪ ♫  ♪ ♫ ♪ Bine ati venit in aplicatia TopMusic ♪ ♫ ♪ ♫ ♪ \n" RESET, query_result[4000];
	char s1[66] = CYAN "Pentru mai multe detalii despre comenzi tastati 'help'\n" RESET;
	strcat(welcome, s1);
	int sd;

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Eroare la socket().\n");
		return errno;
	}

	bzero(&server, sizeof(server));
	bzero(&from, sizeof(from));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(PORT);

	if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
	{
		perror("Eroare la bind().\n");
		return errno;
	}

	if (listen(sd, 1) == -1)
	{
		perror("Eroare la listen().\n");
		return errno;
	}

	while (TRUE)
	{
		int client;
		int length = sizeof(from);

		client = accept(sd, (struct sockaddr *)&from, &length);

		if (client < 0)
		{
			perror("Eroare la accept().\n");
			continue;
		}

		int pid;
		if ((pid = fork()) == -1)
		{
			close(client);
			continue;
		}
		else if (pid > 0)
		{
			close(client);
			while (waitpid(-1, NULL, WNOHANG))
				;
			continue;
		}
		else if (pid == 0)
		{
			// copil, pentru fiecare client
			int logged_in = 0, admin = 0, perm = 0;
			close(sd);
			if (write(client, welcome, strlen(welcome)) <= 0)
			{
				perror("Eroare la scriere catre client.\n");
				exit(0); /* continuam sa ascultam */
			}
			else
				printf("Mesajul welcome fost trasmis cu succes.\n");

			sqlite3 *db;
			int return_code = sqlite3_open("TopMusic.db", &db), cod_msg, uid;

			while (TRUE)
			{
				printf("Asteptam mesajul...\n");
				bzero(msg_received, 300);
				if (read(client, msg_received, 300) <= 0)
				{
					perror("Eroare la citire de la client.\n");
					close(client);
					break;
				}
				printf("Mesajul primit: %s\n", msg_received);

				cod_msg = ret_cod(msg_received);
				switch (cod_msg)
				{
				case -1:
					bzero(msg_sent, 4000);
					sprintf(msg_sent, RED "Exiting" RESET);
					break;
				case 1:
					bzero(msg_sent, 4000);
					if (!logged_in)
					{
						char *err_msg = 0, sql[100], *username = msg_received + 6, *id, copy[100], *p;
						bzero(sql, 100);
						bzero(query_result, 4000);
						sprintf(sql, "SELECT uid,username,type,parm FROM users WHERE username='%s';", username);
						int return_code = sqlite3_exec(db, sql, callback1, query_result, &err_msg);

						if (strlen(query_result))
						{
							strcpy(copy, query_result);
							id = strtok(copy, " ");
							uid = atoi(id);
							p = strtok(NULL, "");
							if (return_code != SQLITE_OK)
							{
								sprintf(msg_sent, "SQL error: %s\n", err_msg);
								sqlite3_free(err_msg);
							}
							else
							{
								//succesful login
								strcpy(msg_sent, GREEN "Welcome! ♫" RESET);
								logged_in = 1;
								if (strstr(query_result, "1 1"))
								{
									admin = 1;
									perm = 1;
								}
								else if (strstr(query_result, "1 0"))
								{
									admin = 1;
									perm = 0;
								}
								else if (strstr(query_result, "0 1"))
								{
									admin = 0;
									perm = 1;
								}
							}
						}
						else
						{
							printf(RED "AICI\n" RESET);
							fflush(stdout);
							strcpy(msg_sent, RED "Username not valid. Please try again or create a new account" RESET);
						}
						bzero(username, strlen(username));
					}
					else
					{
						strcpy(msg_sent, RED "!!! You are already logged in !!!" RESET);
					}

					break;
				case 2:
					if (!logged_in)
					{
						char *err_msg = 0, *user = msg_received + 9, sql[100];
						bzero(sql, 100);
						sprintf(sql, "SELECT username FROM users WHERE username='%s';", user);
						bzero(query_result, 4000);
						return_code = sqlite3_exec(db, sql, callback1, query_result, &err_msg);
						if (return_code != SQLITE_OK)
						{
							sprintf(msg_sent, "SQL error: %s\n", err_msg);
							sqlite3_free(err_msg);
						}
						else if (strstr(query_result, user))
						{
							strcpy(msg_sent, RED "Username already exists! Try logging in ?" RESET);
						}
						else
						{
							bzero(sql, 100);
							sprintf(sql, "INSERT INTO users VALUES ((select max(uid) from users)+1,'%s',0, 1);", user);
							//username=username, type=0 (normal user), vote_permission=1
							bzero(query_result, 4000);
							return_code = sqlite3_exec(db, sql, callback1, query_result, &err_msg);
							if (return_code != SQLITE_OK)
							{
								sprintf(msg_sent, RED "SQL error: %s\n" RESET, err_msg);
								sqlite3_free(err_msg);
							}
							else
								strcpy(msg_sent, GREEN "Registration completed! You can now log in" RESET);
						}
					}
					else
					{
						strcpy(msg_sent, RED "!!! You are already logged in !!!" RESET);
					}

					break;
				case 3:
					bzero(msg_sent, 4000);
					if (admin)
					{
						char sql[300], *err_msg, tokens[5][150], *p = strtok(msg_received + 4, "|");
						bzero(query_result, 4000);
						bzero(sql, 300);
						bzero(tokens, sizeof(tokens));
						int nr = 0;
						while (p)
						{
							strcpy(tokens[nr++], p);
							p = strtok(NULL, "|");
						}
						if (nr > 5 || nr < 3)
						{
							if (nr > 5)
								strcpy(msg_sent, RED "Too many arguments" RESET);
							else
								strcpy(msg_sent, RED "Not enough arguments" RESET);
						}
						else
						{
							bzero(query_result, 4000);
							bzero(sql, 300);
							sprintf(sql, "INSERT INTO songs VALUES((SELECT MAX(sid) FROM SONGS)+1,'%s',0,'%s','%s');", tokens[0], tokens[1], tokens[2]);
							return_code = sqlite3_exec(db, sql, callback1, 0, &err_msg);
							if (return_code != SQLITE_OK)
							{
								sprintf(msg_sent, "SQL error: %s\n", err_msg);
								sqlite3_free(err_msg);
							}
							else
							{
								strcpy(msg_sent, GREEN "Song Added\n" RESET);
							}
							bzero(query_result, 4000);
							bzero(sql, 300);
							if (nr == 5)
							{
								sprintf(sql, "INSERT INTO genres VALUES((SELECT MAX(sid) FROM SONGS),'%s');", tokens[3]);
								return_code = sqlite3_exec(db, sql, callback1, 0, &err_msg);
								bzero(query_result, 4000);
								bzero(sql, 300);
								sprintf(sql, "INSERT INTO genres VALUES((SELECT MAX(sid) FROM SONGS),'%s');", tokens[4]);
								return_code = sqlite3_exec(db, sql, callback1, 0, &err_msg);
								if (return_code != SQLITE_OK)
								{
									sprintf(msg_sent, "SQL error: %s\n", err_msg);
									sqlite3_free(err_msg);
								}
								else
								{
									strcat(msg_sent, GREEN "Genre(s) Added" RESET);
								}
							}
							else if (nr == 4)
							{
								sprintf(sql, "INSERT INTO genres VALUES((SELECT MAX(sid) FROM SONGS),'%s');", tokens[3]);
								return_code = sqlite3_exec(db, sql, callback1, 0, &err_msg);
								if (return_code != SQLITE_OK)
								{
									sprintf(msg_sent, "SQL error: %s\n", err_msg);
									sqlite3_free(err_msg);
								}
								else
								{
									strcat(msg_sent, GREEN "Genre(s) Added" RESET);
								}
							}
						}
					}
					else
					{
						if (logged_in)
							strcpy(msg_sent, YELLOW "You don't have permission" RESET);
						else
							strcpy(msg_sent, YELLOW "You must be logged in" RESET);
					}
					break;
				case 4:
					bzero(msg_sent, 4000);
					if (admin)
					{
						char sql[300], *err_msg, *song_name = msg_received + 7;
						bzero(query_result, 4000);
						bzero(sql, 300);
						sprintf(sql, "SELECT sid FROM songs WHERE name='%s';", song_name);
						return_code = sqlite3_exec(db, sql, callback1, query_result, &err_msg);
						if (return_code != SQLITE_OK)
						{
							sprintf(msg_sent, "SQL error: %s\n", err_msg);
							sqlite3_free(err_msg);
						}
						else if (!strlen(query_result))
						{
							strcpy(msg_sent, RED "Not existing song. Please check the name" RESET);
						}
						else
						{
							bzero(sql, 100);
							sprintf(sql, "DELETE FROM songs where sid=%s", query_result);
							return_code = sqlite3_exec(db, sql, callback1, 0, &err_msg);
							if (return_code != SQLITE_OK)
							{
								sprintf(msg_sent, RED "SQL error: %s\n" RESET, err_msg);
								sqlite3_free(err_msg);
							}
							else
								strcpy(msg_sent, GREEN "Song succesfully deleted ♫" RESET);

							bzero(sql, 100);
							sprintf(sql, "DELETE FROM genres where sid=%s", query_result);
							return_code = sqlite3_exec(db, sql, callback1, 0, &err_msg);
							if (return_code != SQLITE_OK)
							{
								sprintf(msg_sent, RED "SQL error: %s\n" RESET, err_msg);
								sqlite3_free(err_msg);
							}
						}
					}
					else
					{
						if (logged_in)
							strcpy(msg_sent, YELLOW "You don't have permission" RESET);
						else
							strcpy(msg_sent, YELLOW "You must be logged in" RESET);
					}
					break;
				case 5:
					bzero(msg_sent, 4000);
					if (logged_in)
					{
						bzero(query_result, 4000);
						char sql[100], *err_msg, *song_name;
						song_name = msg_received + 5;
						if (perm == 1)
						{
							//User can vote
							bzero(query_result, 4000);
							bzero(sql, 100);
							sprintf(sql, "SELECT sid FROM songs WHERE name='%s';", song_name);
							return_code = sqlite3_exec(db, sql, callback1, query_result, &err_msg);
							if (return_code != SQLITE_OK)
							{
								sprintf(msg_sent, "SQL error: %s\n", err_msg);
								sqlite3_free(err_msg);
							}
							else
							{
								if (query_result[0])
								{
									bzero(sql, 100);
									int n = strlen(query_result);
									query_result[n] = '\0';
									sprintf(sql, "UPDATE songs SET nr_votes = nr_votes + 1 WHERE sid = '%s';", query_result);
									bzero(query_result, 4000);
									return_code = sqlite3_exec(db, sql, callback1, query_result, &err_msg);
									if (return_code != SQLITE_OK)
									{
										sprintf(msg_sent, "SQL error: %s\n", err_msg);
										sqlite3_free(err_msg);
									}
									else
										strcpy(msg_sent, GREEN "Succesfully voted ♫" RESET);
								}
								else
								{
									strcpy(msg_sent, YELLOW "Non existing song" RESET);
								}
							}
						}
						else
							strcpy(msg_sent, YELLOW "You don't have permission to vote!" RESET);
					}
					else
					{
						strcpy(msg_sent, RED "!!! You must be logged in !!!" RESET);
					}

					break;
				case 6:
					bzero(msg_sent, 4000);
					if (admin)
					{

						char *err_msg = 0, sql[150];
						bzero(sql, 150);
						sprintf(sql, "SELECT username FROM users WHERE type=0;");
						bzero(query_result, 4000);

						return_code = sqlite3_exec(db, sql, callback3, query_result, &err_msg);
						if (return_code != SQLITE_OK)
						{
							sprintf(msg_sent, "SQL error: %s\n", err_msg);
							sqlite3_free(err_msg);
						}
						else
						{
							sprintf(msg_sent, CYAN "%s" RESET, query_result);
						}
					}
					else
					{
						if (logged_in)
							strcpy(msg_sent, YELLOW "You don't have permission" RESET);
						else
							strcpy(msg_sent, RED "!!! You must be logged in !!!" RESET);
					}

					break;
				case 7:
					bzero(msg_sent, 4000);
					if (admin)
					{
						char *username = msg_received + 9, sql[100], *err_msg = 0;
						bzero(sql, 100);
						bzero(query_result, 4000);
						sprintf(sql, "SELECT username FROM users WHERE username='%s';", username);
						int return_code = sqlite3_exec(db, sql, callback1, query_result, &err_msg);
						if (strstr(query_result, username))
						{
							bzero(sql, 100);
							bzero(query_result, 4000);
							sprintf(sql, "UPDATE users SET parm=0 WHERE username='%s';", username);
							return_code = sqlite3_exec(db, sql, callback1, query_result, &err_msg);
							if (return_code != SQLITE_OK)
							{
								sprintf(msg_sent, "SQL error: %s\n", err_msg);
								sqlite3_free(err_msg);
							}
							else
								strcpy(msg_sent, GREEN "Succesfully restrcited ♫" RESET);
						}
						else
						{
							strcpy(msg_sent, YELLOW "Not existing username" RESET);
						}

						bzero(username, strlen(username));
					}
					else
					{
						if (logged_in)
							strcpy(msg_sent, YELLOW "You don't have permission" RESET);
						else
							strcpy(msg_sent, RED "!!! You must be logged in !!!" RESET);
					}

					break;
				case 8:
					bzero(msg_sent, 4000);
					if (admin)
					{
						char *err_msg = 0, sql[150];
						bzero(sql, 150);
						sprintf(sql, "SELECT username FROM users WHERE type=1;");
						bzero(query_result, 4000);

						return_code = sqlite3_exec(db, sql, callback3, query_result, &err_msg);
						if (return_code != SQLITE_OK)
						{
							sprintf(msg_sent, "SQL error: %s\n", err_msg);
							sqlite3_free(err_msg);
						}
						else
						{
							sprintf(msg_sent, CYAN "%s" RESET, query_result);
						}
					}
					else
					{
						if (logged_in)
							strcpy(msg_sent, YELLOW "You don't have permission" RESET);
						else
							strcpy(msg_sent, RED "!!! You must be logged in !!!" RESET);
					}
					break;
				case 9:
					bzero(msg_sent, 4000);
					sprintf(msg_sent, CYAN "register <username>\nlogin <username>\nlink <songname>\ngenres\nvote <songname>\ntop [genre]\ncomment <songname>:<message>\ncomm list <songname>\nquit\n" RESET);
					if (admin)
						strcat(msg_sent, YELLOW "\tAdmin only\nrestict <username>\npermit <username>\nlist users\nlist admins\nadd <songname>|<description>|<link>|[<genre1>]|[<genre2>]\ndelete <songname>\n" RESET);
					break;
				case 10:
					bzero(msg_sent, 4000);
					if (logged_in)
					{
						char headers[1000], sql[150], *err_msg = 0, *genre;
						bzero(headers, 1000);
						strcpy(headers, "Name");
						strcat(headers, "                     Votes  Description\n");
						bzero(sql, 150);
						bzero(query_result, 4000);
						printf(BLUE "Getting top\n" RESET);
						fflush(stdout);
						if (strlen(msg_received) >= 4)
						{
							if (msg_received[3] != ' ')
							{
								strcpy(msg_sent, RED "Command not valid\nPlease try again or type help to see a list of commands" RESET);
							}
							else
							{
								genre = msg_received + 4;
								sprintf(sql, "SELECT * FROM genres WHERE genre='%s'", genre);
								return_code = sqlite3_exec(db, sql, callback2, query_result, &err_msg);
								if (return_code != SQLITE_OK)
								{
									sprintf(msg_sent, "SQL error: %s\n", err_msg);
									sqlite3_free(err_msg);
								}
								else
								{
									if (strlen(query_result))
									{
										bzero(sql, 150);
										bzero(query_result, 4000);
										sprintf(sql, "SELECT name,nr_votes,description FROM songs s join genres g on s.sid=g.sid WHERE g.genre='%s' order by s.nr_votes desc;", genre);
										return_code = sqlite3_exec(db, sql, callback2, query_result, &err_msg);
										if (return_code != SQLITE_OK)
										{
											sprintf(msg_sent, "SQL error: %s\n", err_msg);
											sqlite3_free(err_msg);
										}
										else
										{
											strcat(headers, query_result);
											sprintf(msg_sent, BLUE "%s\n" RESET, headers);
										}
									}
									else
									{
										strcpy(msg_sent, RED "Not Existing Genre" RESET);
									}
								}
							}
						}
						else
						{
							strcpy(sql, "SELECT name,nr_votes,description FROM songs ORDER BY nr_votes desc;");
							return_code = sqlite3_exec(db, sql, callback2, query_result, &err_msg);
							if (return_code != SQLITE_OK)
							{
								sprintf(msg_sent, "SQL error: %s\n", err_msg);
								sqlite3_free(err_msg);
							}
							else
							{
								strcat(headers, query_result);
								sprintf(msg_sent, BLUE "%s\n" RESET, headers);
							}
						}
					}
					else
					{
						strcpy(msg_sent, RED "!!! You must be logged in !!!" RESET);
					}
					break;
				case 11:
					bzero(msg_sent, 4000);
					if (logged_in)
					{
						admin = 0;
						logged_in = 0;
						perm = 0;
						strcpy(msg_sent, GREEN "Logout succesfull ♫" RESET);
					}
					else
					{
						strcpy(msg_sent, RED "!!! You must be logged in !!!" RESET);
					}

					break;
				case 12:
					bzero(msg_sent, 4000);
					if (logged_in)
					{
						char *err_msg = 0, *song_name = msg_received + 5, sql[150];
						bzero(sql, 150);
						sprintf(sql, "SELECT link FROM songs WHERE name='%s';", song_name);
						bzero(query_result, 4000);

						return_code = sqlite3_exec(db, sql, callback3, query_result, &err_msg);
						if (return_code != SQLITE_OK)
						{
							sprintf(msg_sent, "SQL error: %s\n", err_msg);
							sqlite3_free(err_msg);
						}
						else
						{
							if (!strlen(query_result))
								sprintf(msg_sent, RED "Not existing name" RESET);
							else
								sprintf(msg_sent, MAGENTA "%s" RESET, query_result);
						}
					}
					else
					{
						strcpy(msg_sent, RED "!!! You must be logged in !!!" RESET);
					}
					break;
				case 13:
					bzero(msg_sent, 4000);
					if (logged_in)
					{
						char *err_msg = 0, sql[150];
						bzero(sql, 150);
						sprintf(sql, "SELECT DISTINCT genre FROM genres;");
						bzero(query_result, 4000);

						return_code = sqlite3_exec(db, sql, callback3, query_result, &err_msg);
						if (return_code != SQLITE_OK)
						{
							sprintf(msg_sent, "SQL error: %s\n", err_msg);
							sqlite3_free(err_msg);
						}
						else
						{
							sprintf(msg_sent, CYAN "%s" RESET, query_result);
						}
					}
					else
					{
						strcpy(msg_sent, RED "!!! You must be logged in !!!" RESET);
					}
					break;
				case 14:
					bzero(msg_sent, 4000);
					if (admin)
					{
						char *username = msg_received + 7, sql[100], *err_msg = 0;
						bzero(sql, 100);
						bzero(query_result, 4000);
						sprintf(sql, "SELECT username FROM users WHERE username='%s';", username);
						int return_code = sqlite3_exec(db, sql, callback1, query_result, &err_msg);
						if (strstr(query_result, username))
						{
							bzero(sql, 100);
							bzero(query_result, 4000);
							sprintf(sql, "UPDATE users SET parm=1 WHERE username='%s';", username);
							return_code = sqlite3_exec(db, sql, callback1, 0, &err_msg);
							if (return_code != SQLITE_OK)
							{
								sprintf(msg_sent, "SQL error: %s\n", err_msg);
								sqlite3_free(err_msg);
							}
							else
								strcpy(msg_sent, GREEN "Succesfully permitted voting ♫" RESET);
						}
						else
						{
							strcpy(msg_sent, RED "Not existing username" RESET);
						}
						bzero(username, strlen(username));
					}
					else
					{
						if (logged_in)
							strcpy(msg_sent, YELLOW "You don't have permission" RESET);
						else
							strcpy(msg_sent, RED "!!! You must be logged in !!!" RESET);
					}

					break;
				case 15:
					bzero(msg_sent, 4000);
					if (logged_in)
					{

						char sql[100], *err_msg, song_name[30], *c;
						if (!strchr(msg_received, ':'))
						{
							strcpy(msg_sent, RED "The sintax for comment is -> comment <songname>:<message>" RESET);
						}
						else
						{

							c = strtok(msg_received + 8, ":");
							strcpy(song_name, c);
							c = strtok(NULL, "");
							bzero(query_result, 4000);
							bzero(sql, 100);
							sprintf(sql, "SELECT name FROM songs WHERE name='%s';", song_name);
							return_code = sqlite3_exec(db, sql, callback1, query_result, &err_msg);
							if (return_code != SQLITE_OK)
							{
								sprintf(msg_sent, "SQL error: %s\n", err_msg);
								sqlite3_free(err_msg);
							}
							else if (strstr(query_result, song_name))
							{

								bzero(sql, 100);
								sprintf(sql, "INSERT INTO comments VALUES(%d,'%s','%s');", uid, song_name, c);
								bzero(query_result, 4000);
								return_code = sqlite3_exec(db, sql, callback1, query_result, &err_msg);
								if (return_code != SQLITE_OK)
								{
									sprintf(msg_sent, "SQL error: %s\n", err_msg);
									sqlite3_free(err_msg);
								}
								else
									strcpy(msg_sent, GREEN "Succesfully added a comment ♫" RESET);
							}
							else
							{
								strcpy(msg_sent, RED "Not existing song" RESET);
							}
						}
					}
					else
					{
						strcpy(msg_sent, RED "!!! You must be logged in !!!" RESET);
					}
					break;
				case 16:
					bzero(msg_sent, 4000);
					if (logged_in)
					{

						char headers[1000], sql[150], *err_msg = 0, *song_name = msg_received + 10;
						bzero(headers, 1000);
						strcpy(headers, "Comments for ");
						bzero(sql, 150);
						bzero(query_result, 4000);
						sprintf(sql, "SELECT name FROM songs WHERE name='%s';", song_name);
						return_code = sqlite3_exec(db, sql, callback1, query_result, &err_msg);

						if (return_code != SQLITE_OK)
						{
							sprintf(msg_sent, "SQL error1: %s\n", err_msg);
							sqlite3_free(err_msg);
						}
						else if (strlen(query_result))
						{
							strcat(headers, song_name);
							strcat(headers, ":\n");

							bzero(query_result, 4000);
							sprintf(sql, "SELECT a.username,b.comm FROM comments b JOIN users a on a.uid=b.uid WHERE b.name='%s';", song_name);
							return_code = sqlite3_exec(db, sql, callback4, query_result, &err_msg);
							if (return_code != SQLITE_OK)
							{
								sprintf(msg_sent, "SQL error2: %s\n", err_msg);
								sqlite3_free(err_msg);
							}
							else
							{
								strcat(headers, query_result);
								sprintf(msg_sent, CYAN "%s\n" RESET, headers);
							}
						}
						else
						{
							strcpy(msg_sent, RED "Not existing song" RESET);
						}
					}
					else
					{
						strcpy(msg_sent, RED "!!! You must be logged in !!!" RESET);
					}
					break;
				default:
					bzero(msg_sent, 4000);
					sprintf(msg_sent, RED "Command not valid\nPlease try again or type help to see a list of commands" RESET);
					break;
				}
				printf("Trimitem mesajul:%s\n", msg_sent);
				if (write(client, msg_sent, strlen(msg_sent)) <= 0)
				{
					perror("Eroare la scriere catre client.\n");
					continue;
				}
				else
					printf("Mesaj trimis\n");
				bzero(msg_sent, 4000);
				if (cod_msg == -1)
				{
					sqlite3_close(db);
					close(client);
					exit(0);
				}
			}
			exit(0);
		}
	}
}