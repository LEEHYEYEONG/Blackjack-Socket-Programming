#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>

#define MAX_CLIENT 3
#define CHATDATA 1024
#define INVALID_SOCK -1

enum shape { spade, club, dia, heart };
enum num { ace = 1, jack = 11, queen = 12, king = 13 };

struct Card {
	int number;
	char shape;
};

struct Player {
	int flag;
	int score;
	int end;
	struct Card card_player[21];
};

struct Card card[52];
struct Player player[MAX_CLIENT];

int list_c[MAX_CLIENT];
int next;
int turns;

char escape[] = "exit";
char start[] = "start";
char hit[] = "hit";
char stay[] = "stay";
char result[] = "result";
char greeting[] = " -------------------------------------- \n| 'start': Game Start                  |\n -------------------------------------- \n";
char exits[] = " -------------------------------------- \n| 'exit': Game Close                   |\n -------------------------------------- \n";

char CODE200[] = "Sorry No More Connection\n";
char chatData[CHATDATA];
char cardData[CHATDATA];
char infoData[CHATDATA];
char sumData[CHATDATA];
char resultData[CHATDATA];

int pushClient(int c_socket);
int popClient(int s);
void fill_card();
void shuffle_card();
const char*  display_card(int p, int turns);
struct Card deal(int next);
const char* hits(int p, int turns);
const char* printHitorStay();
void play();
void reset();
void concat_string(char* dest, const char* src);
const char* stays(int p);

int main(int argc, char *argv[])
{
	int c_socket, s_socket;
	struct sockaddr_in s_addr, c_addr;
	int len;
	int nfds = 0;
	int i, j, n;
	fd_set read_fds;
	int res;

	reset();
	
	if(argc < 2) {
		printf("usage: %s port_number\n", argv[0]);
		exit(-1);
	}
	
	s_socket = socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(atoi(argv[1]));

	if(bind(s_socket, (struct sockaddr *) &s_addr, sizeof(s_addr)) == -1) {
		printf("Can not Bind\n");
		return -1;
	}

	if(listen(s_socket, MAX_CLIENT) == -1) {
		printf("Listen Fail\n");
		return -1;
	}

	for(i = 0; i <MAX_CLIENT; i++)
		list_c[i] = INVALID_SOCK;

	while(1) {
		nfds = s_socket;
		
		FD_ZERO(&read_fds);
		FD_SET(s_socket, &read_fds);

		for(i = 0; i < MAX_CLIENT; i++){
			if(list_c[i] != INVALID_SOCK) {
				FD_SET(list_c[i], &read_fds);
				if(list_c[i] > nfds) nfds = list_c[i];
			}
		}
		nfds++;

		if(select(nfds, &read_fds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0) {
			printf("Select Error\n");
			exit(1);
		}

		if(FD_ISSET(s_socket, &read_fds)) {
			len = sizeof(c_addr);
			if((c_socket = accept(s_socket, (struct sockaddr *) &c_addr, &len)) > 0) {
				res = pushClient(c_socket);
				if(res < 0) {
					write(c_socket, CODE200, strlen(CODE200));
					close(c_socket);
				}else{
					write(c_socket, greeting, strlen(greeting));
					
				}
			}
		}
	
		for(i=0; i < MAX_CLIENT; i++) {
		
			if((list_c[i] != INVALID_SOCK) && FD_ISSET(list_c[i], &read_fds)) {
				memset(chatData, 0, sizeof(chatData));
				if((n = read(list_c[i], chatData, sizeof(chatData))) > 0) {
					for(j = 0; j < MAX_CLIENT; j++) {
						if(list_c[i] != INVALID_SOCK) 
							write(list_c[j], chatData, n);
						}
					
					if((list_c[i]!= INVALID_SOCK)&&(strstr(chatData, start) != NULL)) {
						switch(i)
						{
						case 0:
							player[0].flag = 1;
							player[0].card_player[turns] = deal(next++);
							strcpy(cardData, display_card(0, turns));
							write(list_c[0], cardData, strlen(cardData));
							strcpy(infoData, printHitorStay());
							write(list_c[0], infoData, strlen(infoData)); 
							break;
						case 1:
							player[1].flag = 1;
							player[1].card_player[turns] = deal(next++);
							strcpy(cardData, display_card(1, turns));
							write(list_c[1], cardData, strlen(cardData));
							strcpy(infoData, printHitorStay());
							write(list_c[1], infoData, strlen(infoData));
							break;
						case 2:
							player[2].flag = 1;
							player[2].card_player[turns] = deal(next++);
							strcpy(cardData, display_card(2, turns));
							write(list_c[2], cardData, strlen(cardData));
							strcpy(infoData, printHitorStay());
							write(list_c[2], infoData, strlen(infoData));
							break;
						}
					}
					else if((strstr(chatData, hit) != NULL) &&		
					(player[0].flag == 1 && player[1].flag == 1 && player[2].flag == 1)){
						if(player[i].score < 21){
							strcpy(cardData, hits(i, turns));
							write(list_c[i], cardData, strlen(cardData));
						}
					}
					else if(strstr(chatData, stay) != NULL)
					{
						strcpy(sumData, stays(i));
						write(list_c[i], sumData, strlen(sumData));
					}
					else if((strstr(chatData, result) != NULL) && (player[0].end == 1 && player[1].end == 1 && player[2].end == 1))
					{
						sprintf(resultData, "Final result is Player%d: %d, Player%d: %d, Player%d: %d\n", 1, player[0].score, 2, player[1].score, 3, player[2].score);
						write(list_c[i], resultData, strlen(resultData));
						write(list_c[i], exits, strlen(exits));
					}
					else if(strstr(chatData, escape) != NULL) {
						popClient(list_c[i]);
						break;
					}
							
				}
			}
		}
		}
	return 0;
}

int pushClient(int c_socket) {
	int i;
	for(i = 0; i < MAX_CLIENT; i++) {
		if(list_c[i] == INVALID_SOCK) {
			list_c[i] = c_socket;
			return i;
		}
	}
	if(i == MAX_CLIENT)
		return -1;
}

int popClient(int s)
{
	int i;
	close(s);
	for(i = 0; i < MAX_CLIENT; i++) {
		if(s == list_c[i]) {
			list_c[i] = INVALID_SOCK;
			break;
		}
	}
	return 0;
}

void fill_card()
{
	int i = 0;
	for(int j = 0; j < 4; j++){
		for(int k = 1; k < 14; k++)
		{
			card[i].number = k;
			card[i].shape = j;
			i++;
		}
	}	
}

void shuffle_card()
{
	srand(time(NULL));
	for(int i = 0; i < 52; i++)
	{

		int index = rand() % 52;
		struct Card temp = card[index];
		card[index] = card[i];
		card[i] = temp;
	}
}

const char* display_card(int p, int turns)
{
	static char cardData1[CHATDATA];
	switch(player[p].card_player[turns].shape)
	{
	case spade:
		strcpy(cardData1, "SPADE");
		break;
	case dia:
		strcpy(cardData1, "DIA");
		break;
	case heart:
		strcpy(cardData1, "HEART");
		break;
	case club:
		strcpy(cardData1, "CLUB");
		break;
	}

	switch(player[p].card_player[turns].number)
	{
		case ace:
			concat_string(cardData1, " A\n");
			player[p].score += 1;
			break;
		case jack:
			concat_string(cardData1, " J\n");
			player[p].score += 10;
			break;
		case queen:
			concat_string(cardData1, " Q\n");
			player[p].score += 10;
			break;
		case king:
			concat_string(cardData1, " K\n");
			player[p].score += 10;
			break;
		default:
			char buf1[CHATDATA];
			sprintf(buf1, "%2d\n", player[p].card_player[turns].number);
			concat_string(cardData1, buf1);
			player[p].score += player[p].card_player[turns].number;
	}
	return cardData1;
}
		
struct Card deal(int next)
{
	return card[next];
}

		 
const char*  hits(int p, int turns)
{
	static char cardData2[CHATDATA];	
	turns++;
	player[p].card_player[turns] = deal(next++);
	strcpy(cardData2, display_card(p, turns));
	
	if(player[p].score > 21)
	{
		sprintf(cardData2, "Player%d is Bust\nResult is %d\n", p+1, player[p].score);
		player[p].end = 1;
	}
	else if(player[p].score == 21)
	{
		sprintf(cardData2, "Player%d is Win\n", p+1);
		player[p].end = 1;
	}
	return cardData2;
}

const char* stays(int p)
{
	static char sumData1[CHATDATA];
	sprintf(sumData1, "Result is %d\n", player[p].score);
	player[p].end = 1;
	
	return sumData1;
}	

void reset()
{
	player[0].score = 0;
	player[1].score = 0;
	player[2].score = 0;

	fill_card();
	shuffle_card();
	next = 0;
	turns = 0;
}

void concat_string(char* dest, const char* src){
	dest = dest + strlen(dest);

	while(*src != '\0')
	{
		*dest = *src;
		dest++;
		src++;
	}
	*dest = '\0';
}
	
const char*  printHitorStay()
{
	static char hitorstay[] = " ------------------------------------------ \n| 'hit': Receive One More Card              |\n| 'stay': Wait without Receiving a Card     |\n ------------------------------------------- \n";
	return hitorstay;
}
