#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int initialization(int argc, char **argv);
void teardown();
char input(int turn, int argc);
void update(char input, int turn, int argc);
void display();
void exit();
int checkSq(char *board, int length, int currHeight, int currLength, char target);      /* Function to check if the char value of a particular cell in a 2D-array 
                                                                                            matches a specified char - char target */
char inputRedo(int turn, int col, int argc, int serversPlayer);

int serversPlayer;       /*Decider-value to choose Player 1 and Player 2 between Server and Client - 50% random chance 
                          0 = player1 (turn 1), 1 = player 2 (turn 2) */
char player1[50];       //Player's name, their color will be red - R
char player2[50];       //Player's name, their color will be black - B
int length = 7;
int height = 6;

int winner = 0;
int loser = 0;
int quitter = 0;            //for if a player chooses quit, the opposite player is the winner

int turn = 0;           //for the player whose turn it is

int gameWon = 0;
int gameQuit = 0;

int boardFull = 0;      //0 if board is not full, 1 if board is full
char *board;
int rowsRemaining = 7;      //board begins with 7 rows, rowsRemaining - 1 when a row fills up until 0 

int sockfd;             //global file descriptor used for communciation by both Server-side and Client-side

int main(int argc, char **argv){

    char inp;   //input returned from input(), passed to update()

    sockfd = initialization(argc, argv);

    display();           //call display() one time to see board for the first turn

    while(gameWon == 0 && gameQuit == 0)
    {
        turn %= 2;      //turn = 1 for player 1, turn = 2 for player 2
        turn++;

        inp = input(turn, argc);
        update(inp, turn, argc);
        display();

        if(gameQuit == 1)
        {
            printf("---------- Game Over: The game has been quit ----------\n\n");
            if(quitter == 1)
            {
                printf("%s has quit the game\n\n", player1);
                printf("Winner: %s\n", player2);
                printf("Loser: %s", player1);
            }
            else if(quitter == 2)
            {
                printf("%s has quit the game\n\n", player2);
                printf("Winner: %s\n", player1);
                printf("Loser: %s", player2);
            }

            printf("\n\nGood job players:\n\t%s\n\t%s", player1, player2);
        }

        else if(gameWon != 0)
        {
            printf("---------- Game Over: The game has been won ----------\n\n");
            if(winner == 1)
            {
                printf("Winner: %s\n", player1);
                printf("Loser: %s", player2);
            }
            else if(winner == 2)
            {
                printf("Winner: %s\n", player2);
                printf("Loser: %s", player1);
            }
            
            printf("\n\nGood job players:\n\t%s\n\t%s", player1, player2);
        }

        else if(boardFull == 1)
        {
            printf("---------- Game Over: The board is full -----------\n");
            printf("\n\t\tThe game is a tie\n\n");
            printf("Good job players:\n\t%s\n\t%s", player1, player2);
            teardown();
        }

    }

    teardown();

    return 0;
}



int initialization(int argc, char **argv) {

if(argc != 2 && argc != 3)
{
    /* Invalid command-line arguments */

    printf("\n\nError: Invalid command-line arguments\n\n\n");
    printf("Command-line arguments must either be 1 for Server-mode or 2 for Client-mode.\n\n");
    printf("Example:\nServer-mode: ./[output file] [port]\nClient-mode: ./[output file] [IP address] [port]\n\n\n");
    exit(1);
}

board = (char *)malloc(length * height * sizeof(char));     //create the game board

int index;                                      //Index through the board and set all slots to empty, '_'
for(int i = 0; i < height; i ++)                //For personal reference: 2D-array must be interpreted ROWS FIRST, COLUMNS SECOND
{                                               //                         Therefore, double for-loop goes height first, length second
    for(int j = 0; j < length; j++)             //                         Not length first, height second
    {                                           //Not doing such does not change the number of array indices, however the interpreted number value of
        index = i * length + j;                 //the indices changes.
        (*(board + index)) = '_';               //Also - (*(board + index)) in this for loop would be equivalent to saying: 
    }                                           //       for(int i = 0; i < length * height; i++) { board[i] }
}

if(argc == 2)
{
    /* Server Mode */

    printf("\n\nYou are running as: Server-mode\n\n\n");

    /* Set up connection and wait for someone to connect */

    const char *port = argv[1];     //server port to be used (command line arg for running client)

    const int BACKLOG = 10;
    int new_fd;
    int sockfd;
    int rv;

    struct addrinfo hints, *servinfo, *p;   
    struct sockaddr_storage their_addr;   
    socklen_t size;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;    //use my IP

    if((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)       //get address info that matches hints and store it in servinfo
    {
        printf("\nError: getaddrinfo() on server-side failed.\n");
        printf("Error value: %d", rv);
    }

    int bindval;
    for(p = servinfo; p != NULL; p = p->ai_next)        //loop through all pending addresses that match hints
    {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)   //create socket
        {
            printf("Error: socket() on server-side failed.\nError value: %d\n", sockfd);
            continue;
        }
        if((bindval = bind(sockfd, p->ai_addr, p->ai_addrlen)) == -1)         //bind address to socket
        {
            printf("Error: bind() failed.\nError value: %d\n", bindval);
            close(sockfd);
            continue;
        }

    }

    freeaddrinfo(servinfo);     //free network resources

    int listenval;
    if((listenval = listen(sockfd, BACKLOG)) == -1)     //listen for incoming connection requests
    {
        printf("Error: listen() failed.\nError value: %d\n", listenval);
        exit(1);
    }

    int connected = 0;
    while(connected != 1)
    {
        size = sizeof(their_addr);
        if((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &size)) == -1)      //accept incoming request and create new socket for communication
        {
            printf("Error: accept() failed.\nError value: %d\n", new_fd);
        }
        else
        {
            printf("Client has connected to the game.\n\n\n");
            connected++;
        }
    }

    /* Randomly (50% chance) decide either Player 1 or Player 2 for Server-side */
    int random;
    time_t t;
    srand((unsigned) time(&t));        //seed rand()
    random = rand() % 2;

    if(random == 0)
    {
        serversPlayer = 1;
    }
    else if(random == 1)
    {
        serversPlayer = 2;
    }

    /* Send random choice to Client-side (Client-side will take the other choice) */
    if(send(new_fd, &serversPlayer, sizeof(serversPlayer), 0) == -1)
    {
        printf("Error: send() on Server-side failed.");
    }

    if(serversPlayer == 1)
    {
        printf("\nWelcome to Connect Four.\n\n--------------- Setting up the game ---------------\n\n");
        printf("\nPlayer 1, enter your name...\n\n");
        scanf("%s",player1);
        printf("\n%s: Your color is red - R\n\n\n", player1);

        if(send(new_fd, player1, sizeof(player1), 0) == -1)     //send name if Player 1
        {
            printf("\nError: initialization() > send() > player1 on Server-side failed.\n");
        }
        if(recv(new_fd, player2, sizeof(player2), 0) == -1)     //receive player 2 name from Client
        {
            printf("\nError: initialization() > recv() > player2 on Server-side failed.\n");
        }
    }
    else if(serversPlayer == 2)
    {
        printf("\nWelcome to Connect Four.\n\n--------------- Setting up the game ---------------\n\n");
        printf("\nPlayer 2, enter your name...\n\n");
        scanf("%s",player2);
        printf("\n%s: Your color is black - B\n\n\n", player2);

        if(send(new_fd, player2, sizeof(player2), 0) == -1)     //send name if Player 2
        {
            printf("\nError: initialization() > send() > player2 on Server-side failed.\n");
        }
        if(recv(new_fd, player1, sizeof(player1), 0) == -1)     //receive player 1 name from Client
        {
            printf("\nError: initialization() > recv() > player1 on Server-side failed.\n");
        }
    }


    return new_fd;      //return file descriptor used for communication 

}       /* End Server-side initialization */

else        // equal to (argc == 3)
{
    /* Client Mode */

    printf("\n\nYou are running as: Client-mode\n\n\n");

    /* Connect to the server */

    char *address = argv[1];
    char *port = argv[2];

    int sockfd, numbytes, rv;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));       // set up hints
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;
        
    if((rv = getaddrinfo(address, port, &hints, &servinfo)) != 0)      //get address info that matches hints and store it in servinfo
    {        
        printf("\nError: getaddrinfo() on client-side failed.\n");
        printf("Error value: %d", rv);
    }

    int connectval;
    for(p = servinfo; p != NULL; p = p->ai_next)            //loop through all pending addresses that match hints
    {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)      //create socket
        {
            printf("Error: socket() on client-side failed.\nError value: %d\n", sockfd);
            continue;
        }
        if((connectval = connect(sockfd, p->ai_addr, p->ai_addrlen)) == -1)     //connect socket to the server address
        {
            printf("Error: connect() failed.\nError value: %d\n", connectval);
            continue;
        }
        else
        {
            printf("Connection to server established.\n\n\n");
        }
    }

    freeaddrinfo(servinfo);     // free network resources

    /* Receive random choice from Server-side - Reverse the name setups for serversPlayer compared to Server-side */
    if(recv(sockfd, &serversPlayer, sizeof(serversPlayer), 0) == -1)
    {
        printf("Error: recv() on Client-side failed.");
    } 

    if(serversPlayer == 1)
    {
        printf("\nWelcome to Connect Four.\n\n--------------- Setting up the game ---------------\n\n");
        printf("\nPlayer 2, enter your name...\n\n");
        scanf("%s",player2);
        printf("\n%s: Your color is black - B\n\n\n", player2);

        if(send(sockfd, player2, sizeof(player2), 0) == -1)     //send name if Player 2
        {
            printf("\nError: initialization() > send() > player2 on Client-side failed.\n");
        }
        if(recv(sockfd, player1, sizeof(player1), 0) == -1)     //receive player 1 name from Server
        {
            printf("\nError: initialization() > recv() > player1 on Client-side failed.\n");
        }
    }
    else if(serversPlayer == 2)
    {
        printf("\nWelcome to Connect Four.\n\n--------------- Setting up the game ---------------\n\n");
        printf("\nPlayer 1, enter your name...\n\n");
        scanf("%s",player1);
        printf("\n%s: Your color is red - R\n\n\n", player1);

        if(send(sockfd, player1, sizeof(player1), 0) == -1)     //send name if Player 1
        {
            printf("\nError: initialization() > send() > player1 on Client-side failed.\n");
        }
        if(recv(sockfd, player2, sizeof(player2), 0) == -1)     //receive player 2 name from Server
        {
            printf("\nError: initialization() > recv() > player2 on Client-side failed.\n");
        }
    }

    return sockfd;      //return file descriptor used for communication

}      /* End Client-side initialization */

}       /* End initialization() */


void teardown() {
    printf("\n\n--------------- Destroying the game ---------------\n\n\n\n\n");

    free(board);    //free game board memory from malloc()

    shutdown(sockfd, 2);    //close sockfd, stop any transmission or reception data waiting to be transmitted.

    exit(0);        //Game over, exit the program
}


char input(int turn, int argc) {

  char input;
  char * inp = &input;

  if(argc == 2)
  {
      /* input() for Server-mode */

    if(turn == 1 && serversPlayer == 1)
    {
        /* Player 1's turn AND Server is Player 1 */

        printf("---------------------------------\n\n");
        printf("Turn: %s", player1);
        printf("\n\nEnter a letter A-G, for the column to drop a disc into...");
        printf("\nOr press 'Q' to quit the game.\n\n");

        scanf("%s", &input);

        while(input != 'A' && input != 'B' && input != 'C' && input != 'D' && input != 'E' 
        && input != 'F' && input != 'G' && input != 'Q')
        {
            printf("\nInvalid move.\nLetter entered must either be A-G or Q.\n\n");
            printf("Enter a letter, or press Q to quit.\n\n");
            scanf("%s", &input);
        }

        if(send(sockfd, inp, sizeof(inp), 0) == -1)         //send input to client
        {
            printf("\nError: input() > send() on Server-side failed.\n");
        }

    }
    else if(turn == 2 && serversPlayer == 2)
    {
        /* Player 2's turn AND Server is Player 2 */

        printf("---------------------------------\n\n");
        printf("Turn: %s", player2);
        printf("\n\nEnter a letter A-G, for the column to drop a disc into...");
        printf("\nOr press 'Q' to quit the game.\n\n");

        scanf("%s", &input);
        
        while(input != 'A' && input != 'B' && input != 'C' && input != 'D' && input != 'E' 
        && input != 'F' && input != 'G' && input != 'Q')
        {
            printf("\nInvalid move.\nLetter entered must either be A-G or Q.\n\n");
            printf("Enter a letter, or press Q to quit.\n\n");
            scanf("%s", &input);
        }

        if(send(sockfd, inp, sizeof(inp), 0) == -1)         //send input to client
        {
            printf("\nError: input() > send() on Server-side failed.\n");
        }

    }
    else if(turn == 1 && serversPlayer == 2 || turn == 2 && serversPlayer == 1)
    {
        /* It is not the Server-mode player's turn, regardless of whether it is the turn for player 1 or player 2 
           Note: turn == 1: player 1's turn    turn == 2: player 2's turn 
                 serversPlayer == 1: player 1      serversPlayer == 2: player 2 */

        if(serversPlayer == 1)
        {
            printf("\n\n\n%s's turn - waiting for their move...\n\n\n", player2);
        }
        else if(serversPlayer == 2)
        {
            printf("\n\n\n%s's turn - waiting for their move...\n\n\n", player1);
        }

        if(recv(sockfd, inp, sizeof(inp), 0) == -1)         //receive input from client
        {
            printf("\nError: input() > recv() on Server-side failed.\n");
        }

    }

  }

  else if(argc == 3)
  {
      /* input() for Client-mode */

    if(turn == 1 && serversPlayer == 2)
    {
        /* Player 1's turn AND Client is Player 1 */

        printf("---------------------------------\n\n");
        printf("Turn: %s", player1);
        printf("\n\nEnter a letter A-G, for the column to drop a disc into...");
        printf("\nOr press 'Q' to quit the game.\n\n");

        scanf("%s", &input);
        
        while(input != 'A' && input != 'B' && input != 'C' && input != 'D' && input != 'E' 
        && input != 'F' && input != 'G' && input != 'Q')
        {
            printf("\nInvalid move.\nLetter entered must either be A-G or Q.\n\n");
            printf("Enter a letter, or press Q to quit.\n\n");
            scanf("%s", &input);
        }

        if(send(sockfd, inp, sizeof(inp), 0) == -1)
        {
            printf("\nError: input() > send() on Client-side failed.\n");
        }

    }
    else if(turn == 2 && serversPlayer == 1)
    {
        /* Player 2's turn AND Client is Player 2 */

        printf("---------------------------------\n\n");
        printf("Turn: %s", player2);
        printf("\n\nEnter a letter A-G, for the column to drop a disc into...");
        printf("\nOr press 'Q' to quit the game.\n\n");

        scanf("%s", &input);
        
        while(input != 'A' && input != 'B' && input != 'C' && input != 'D' && input != 'E' 
        && input != 'F' && input != 'G' && input != 'Q')
        {
            printf("\nInvalid move.\nLetter entered must either be A-G or Q.\n\n");
            printf("Enter a letter, or press Q to quit.\n\n");
            scanf("%s", &input);
        }

        if(send(sockfd, inp, sizeof(inp), 0) == -1)
        {
            printf("\nError: input() > send() on Client-side failed.\n");
        }

    }
    else if(turn == 1 && serversPlayer == 1 || turn == 2 && serversPlayer == 2)
    {
        /* It is not the Client-mode player's turn, regardless of whether it is the turn for player 1 or player 2 
           Note: turn == 1: player 1's turn    turn == 2: player 2's turn 
                 serversPlayer == 1: player 1      serversPlayer == 2: player 2 */

        if(serversPlayer == 1)
        {
            printf("\n\n\n%s's turn - waiting for their move...\n\n\n", player1);
        }
        else if(serversPlayer == 2)
        {
            printf("\n\n\n%s's turn - waiting for their move...\n\n\n", player2);
        }

        if(recv(sockfd, inp, sizeof(inp), 0) == -1)         //receive input from server
        {
            printf("\nError: input() > recv() on Client-side failed.\n");
        }
    }

  }
    
    printf("Column chosen: %c\n", input);
    printf("----------------\n\n");

    return input;
}


void update(char input, int turn, int argc) {
    
    int i;
    
    if(turn == 1)                //update based on if player1
    {
    switch(input)
    {
        case 'A':
            i = 0;
            while((*(board + (i * length + 0))) != '_' && i < (height-1))     //fine next empty row in a column
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 0))) != '_')
            {
                update(inputRedo(1, 1, argc, serversPlayer), 1, argc);
            }
            else                                        // i is the next empty row of the chosen column
            {
                (*(board + (i * length + 0))) = 'R';     //R or B for whichever player
            }

            if((*(board + ((height - 1) * length + 0))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'B':
            i = 0;
            while((*(board + (i * length + 1))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 1))) != '_')
            {
                update(inputRedo(1, 2, argc, serversPlayer), 1, argc);
            }
            else                                        
            {
                (*(board + (i * length + 1))) = 'R';     
            }
            if((*(board + ((height - 1) * length + 1))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'C':
            i = 0;
            while((*(board + (i * length + 2))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 2))) != '_')
            {
                update(inputRedo(1, 3, argc, serversPlayer), 1, argc);
            }
            else                                        
            {
                (*(board + (i * length + 2))) = 'R';     
            }
            if((*(board + ((height - 1) * length + 2))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'D':
            i = 0;
            while((*(board + (i * length + 3))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 3))) != '_')
            {
                update(inputRedo(1, 4, argc, serversPlayer), 1, argc);
            }
            else                                        
            {
                (*(board + (i * length + 3))) = 'R';     
            }
            if((*(board + ((height - 1) * length + 3))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'E':
            i = 0;
            while((*(board + (i * length + 4))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 4))) != '_')
            {
                update(inputRedo(1, 5, argc, serversPlayer), 1, argc);
            }
            else                                        
            {
                (*(board + (i * length + 4))) = 'R';     
            }
            if((*(board + ((height - 1) * length + 4))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'F':
            i = 0;
            while((*(board + (i * length + 5))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 5))) != '_')
            {
                update(inputRedo(1, 6, argc, serversPlayer), 1, argc);
            }
            else                                        
            {
                (*(board + (i * length + 5))) = 'R';     
            }
            if((*(board + ((height - 1) * length + 5))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'G':
            i = 0;
            while((*(board + (i * length + 6))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 6))) != '_')
            {
                update(inputRedo(1, 7, argc, serversPlayer), 1, argc);
            }
            else                                        
            {
                (*(board + (i * length + 6))) = 'R';     
            }
            if((*(board + ((height - 1) * length + 6))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'Q':
            gameWon = 1;
            gameQuit = 1;
            winner = 2;
            quitter = 1;
            break;
    }    
    }

    else if(turn == 2)           //update based on if player2
    {
    switch(input)
    {
        case 'A':
            i = 0;
            while((*(board + (i * length + 0))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 0))) != '_')
            {
                update(inputRedo(2, 1, argc, serversPlayer), 2, argc);
            }
            else                                        
            {
                (*(board + (i * length + 0))) = 'B';     
            }
            if((*(board + ((height - 1) * length + 0))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'B':
            i = 0;
            while((*(board + (i * length + 1))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 1))) != '_')
            {
                update(inputRedo(2, 2, argc, serversPlayer), 2, argc);
            }
            else                                        
            {
                (*(board + (i * length + 1))) = 'B';     
            }
            if((*(board + ((height - 1) * length + 1))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'C':
            i = 0;
            while((*(board + (i * length + 2))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 2))) != '_')
            {
                update(inputRedo(2, 3, argc, serversPlayer), 2, argc);
            }
            else                                        
            {
                (*(board + (i * length + 2))) = 'B';     
            }
            if((*(board + ((height - 1) * length + 2))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'D':
            i = 0;
            while((*(board + (i * length + 3))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 3))) != '_')
            {
                update(inputRedo(2, 4, argc, serversPlayer), 2, argc);
            }
            else                                        
            {
                (*(board + (i * length + 3))) = 'B';     
            }
            if((*(board + ((height - 1) * length + 3))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'E':
            i = 0;
            while((*(board + (i * length + 4))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 4))) != '_')
            {
                update(inputRedo(2, 5, argc, serversPlayer), 2, argc);
            }
            else                                        
            {
                (*(board + (i * length + 4))) = 'B';     
            }
            if((*(board + ((height - 1) * length + 4))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'F':
            i = 0;
            while((*(board + (i * length + 5))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 5))) != '_')
            {
                update(inputRedo(2, 6, argc, serversPlayer), 2, argc);
            }
            else                                        
            {
                (*(board + (i * length + 5))) = 'B';     
            }
            if((*(board + ((height - 1) * length + 5))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'G':
            i = 0;
            while((*(board + (i * length + 6))) != '_' && i < (height-1))     
            {
                i++;
            }

            if(i == (height - 1) && (*(board + ((height - 1) * length + 6))) != '_')
            {
                update(inputRedo(2, 7, argc, serversPlayer), 2, argc);
            }
            else                                        
            {
                (*(board + (i * length + 6))) = 'B';     
            }
            if((*(board + ((height - 1) * length + 6))) != '_')
            {
                rowsRemaining--;
            }

            break;

        case 'Q':
            gameWon = 1;
            gameQuit = 1;
            winner = 1;
            quitter = 2;
            break;
    }  
    }

//--------------------------------- Begin check for 4-in-a-row algorithm ---------------------------------

    int index;

    int north1 = 0;                 //direction flags for player 1
    int northEast1 = 0;
    int east1 = 0;
    int southEast1 = 0;
    int south1 = 0;
    int southWest1 = 0;
    int west1 = 0;
    int northWest1 = 0;

    int north2 = 0;                 //direction flags for player 2
    int northEast2 = 0;
    int east2 = 0;
    int southEast2 = 0;
    int south2 = 0;
    int southWest2 = 0;
    int west2 = 0;
    int northWest2 = 0;


    if(turn == 1)           //check if player1's last move matched 4 disks in a row
    {
        for(int i = 0; i < height; i++)
        {
            for(int j = 0; j < length; j++)
            {
                index = i * length + j;

                if(board[index] == 'R')
                {
                    /* Check in all 8 directions surrounding the current index for 4 in a row
                        If statements - first test if coordinates i and j are in bound, then tests
                        if char at the tests square matches the player's disk, using the
                        checkSq() function. */

                    //check north

                    if((i+1) < height && checkSq(board, length, (i+1), j, 'R' ) == 1)
                    {
                        if((i+2) < height && checkSq(board, length, (i+2), j, 'R') == 1)
                        {
                            if((i+3) < height && checkSq(board, length, (i+3), j, 'R') == 1)
                            {
                                north1++;
                                gameWon = 1;
                                winner = 1;
                                loser = 2;
                            }
                        }
                    }

                    //check north east

                    if((i+1) < height && (j+1) < length && checkSq(board, length, (i+1), (j+1), 'R') == 1)
                    {
                        if((i+2) < height && (j+2) < length && checkSq(board, length, (i+2), (j+2), 'R') == 1)
                        {
                            if((i+3) < height && (j+3) < length && checkSq(board, length, (i+3), (j+3), 'R') == 1)
                            {
                                northEast1++;
                                gameWon = 1;
                                winner = 1;
                                loser = 2;
                            }
                        }
                    }

                    //Check east

                    if((j+1) < length && checkSq(board, length, i, (j+1), 'R') == 1)
                    {
                        if((j+2) < length && checkSq(board, length, i, (j+2), 'R') == 1)
                        {
                            if((j+3) < length && checkSq(board, length, i, (j+3), 'R') == 1)
                            {
                                east1++;
                                gameWon = 1;
                                winner = 1;
                                loser = 2;
                            }
                        }
                    }

                    //check south east

                    if((i-1) >= 0 && (j+1) < length && checkSq(board, length, (i-1), (j+1), 'R') == 1)
                    {
                        if((i-2) >= 0 && (j+2) < length && checkSq(board, length, (i-2), (j+2), 'R') == 1)
                        {
                            if((i-3) >= 0 && (j+3) < length && checkSq(board, length, (i-3), (j+3), 'R') == 1)
                            {
                                southEast1++;
                                gameWon = 1;
                                winner = 1;
                                loser = 2;
                            }
                        }
                    }

                    //check south

                    if((i-1) >= 0 && checkSq(board, length, (i-1), j, 'R') == 1)
                    {
                        if((i-2) >= 0 && checkSq(board, length, (i-2), j, 'R') == 1)
                        {
                            if((i-3) >= 0 && checkSq(board, length, (i-3), j, 'R') == 1)
                            {
                                south1++;
                                gameWon = 1;
                                winner = 1;
                                loser = 2;
                            }
                        }
                    }

                    //check south west

                    if((i-1) >= 0 && (j-1) >= 0 && checkSq(board, length, (i-1), (j-1), 'R') == 1)
                    {
                        if((i-2) >= 0 && (j-2) >= 0 && checkSq(board, length, (i-2), (j-2), 'R') == 1)
                        {
                            if((i-3) >= 0 && (j-3) >= 0 && checkSq(board, length, (i-3), (j-3), 'R') == 1)
                            {
                                southWest1++;
                                gameWon = 1;
                                winner = 1;
                                loser = 2;
                            }
                        }
                    }

                    //check west

                    if((j-1) >= 0 && checkSq(board, length, i, (j-1), 'R') == 1)
                    {
                        if((j-2) >= 0 && checkSq(board, length, i, (j-2), 'R') == 1)
                        {
                            if((j-3) >= 0 && checkSq(board, length, i, (j-3), 'R') == 1)
                            {
                                west1++;
                                gameWon = 1;
                                winner = 1;
                                loser = 2;
                            }
                        }
                    }

                    //check north west

                    if((i+1) < height && (j-1) >= 0 && checkSq(board, length, (i+1), (j-1), 'R') == 1)
                    {
                        if((i+2) < height && (j-2) >= 0 && checkSq(board, length, (i+2), (j-2), 'R') == 1)
                        {
                            if((i+3) < height && (j-3) >= 0 && checkSq(board, length, (i+3), (j-3), 'R') == 1)
                            {
                                northWest1++;
                                gameWon = 1;
                                winner = 1;
                                loser = 2;
                            }
                        }
                    }

                    

                }

            }
        }
    }

    if(turn == 2)           //check if player2's last move matched 4 disks in a row
    {
        for(int i = 0; i < height; i++)
        {
            for(int j = 0; j < length; j++)
            {
                index = i * length + j;

                if(board[index] == 'B')
                {
                    //check around it (all 8 squares) in all directions

                    //check north

                    if((i+1) < height && checkSq(board, length, (i+1), j, 'B' ) == 1)
                    {
                        if((i+2) < height && checkSq(board, length, (i+2), j, 'B') == 1)
                        {
                            if((i+3) < height && checkSq(board, length, (i+3), j, 'B') == 1)
                            {
                                north2++;
                                gameWon = 1;
                                winner = 2;
                                loser = 1;
                            }
                        }
                    }

                    //check north east

                    if((i+1) < height && (j+1) < length && checkSq(board, length, (i+1), (j+1), 'B') == 1)
                    {
                        if((i+2) < height && (j+2) < length && checkSq(board, length, (i+2), (j+2), 'B') == 1)
                        {
                            if((i+3) < height && (j+3) < length && checkSq(board, length, (i+3), (j+3), 'B') == 1)
                            {
                                northEast2++;
                                gameWon = 1;
                                winner = 2;
                                loser = 1;
                            }
                        }
                    }

                    //Check east

                    if((j+1) < length && checkSq(board, length, i, (j+1), 'B') == 1)
                    {
                        if((j+2) < length && checkSq(board, length, i, (j+2), 'B') == 1)
                        {
                            if((j+3) < length && checkSq(board, length, i, (j+3), 'B') == 1)
                            {
                                east2++;
                                gameWon = 1;
                                winner = 2;
                                loser = 1;
                            }
                        }
                    }

                    //check south east

                    if((i-1) >= 0 && (j+1) < length && checkSq(board, length, (i-1), (j+1), 'B') == 1)
                    {
                        if((i-2) >= 0 && (j+2) < length && checkSq(board, length, (i-2), (j+2), 'B') == 1)
                        {
                            if((i-3) >= 0 && (j+3) < length && checkSq(board, length, (i-3), (j+3), 'B') == 1)
                            {
                                southEast2++;
                                gameWon = 1;
                                winner = 2;
                                loser = 1;
                            }
                        }
                    }

                    //check south

                    if((i-1) >= 0 && checkSq(board, length, (i-1), j, 'B') == 1)
                    {
                        if((i-2) >= 0 && checkSq(board, length, (i-2), j, 'B') == 1)
                        {
                            if((i-3) >= 0 && checkSq(board, length, (i-3), j, 'B') == 1)
                            {
                                south2++;
                                gameWon = 1;
                                winner = 2;
                                loser = 1;
                            }
                        }
                    }

                    //check south west

                    if((i-1) >= 0 && (j-1) >= 0 && checkSq(board, length, (i-1), (j-1), 'B') == 1)
                    {
                        if((i-2) >= 0 && (j-2) >= 0 && checkSq(board, length, (i-2), (j-2), 'B') == 1)
                        {
                            if((i-3) >= 0 && (j-3) >= 0 && checkSq(board, length, (i-3), (j-3), 'B') == 1)
                            {
                                southWest2++;
                                gameWon = 1;
                                winner = 2;
                                loser = 1;
                            }
                        }
                    }

                    //check west

                    if((j-1) >= 0 && checkSq(board, length, i, (j-1), 'B') == 1)
                    {
                        if((j-2) >= 0 && checkSq(board, length, i, (j-2), 'B') == 1)
                        {
                            if((j-3) >= 0 && checkSq(board, length, i, (j-3), 'B') == 1)
                            {
                                west2++;
                                gameWon = 1;
                                winner = 2;
                                loser = 1;
                            }
                        }
                    }

                    //check north west

                    if((i+1) < height && (j-1) >= 0 && checkSq(board, length, (i+1), (j-1), 'B') == 1)
                    {
                        if((i+2) < height && (j-2) >= 0 && checkSq(board, length, (i+2), (j-2), 'B') == 1)
                        {
                            if((i+3) < height && (j-3) >= 0 && checkSq(board, length, (i+3), (j-3), 'B') == 1)
                            {
                                northWest2++;
                                gameWon = 1;
                                winner = 2;
                                loser = 1;
                            }
                        }
                    }

                    

                }

            }
        }
    }


//--------------------------------- End check for 4-in-a-row algorithm ---------------------------------



    if(rowsRemaining == 0)        // == 0 if no rows remain
    {
        boardFull = 1;
    }
}


void display() {

    /* Loop though the entire board - print [_] for a square that is not taken, [R] for a square
       taken by Player 1, or [B] for a square taken by Player 2 */

    int index;

    printf(" A    B    C    D    E    F    G \n");
    printf("---------------------------------");

    for(int i = height; i >= 0; i--)
    {
        for(int j = 0; j < length; j++)
        {
            index = i * length + j;

            switch(board[index])
            {
                case '_':
                    printf("[_]  ");
                break;

                case 'R':
                    printf("[R]  ");
                break;

                case 'B':
                    printf("[B]  ");
                break;
            }
        }

        printf("\n\n");
    }




}


int checkSq(char *board, int length, int currHeight, int currLength, char target){
   
    /* Compare char at the current index to char target - return 1 if equal, 0 if not equal */

   int index = currHeight * length + currLength;
   
    if(board[index] == target)
    {
        return 1;
    }
    else
    {
        return 0;
    }

}


char inputRedo(int turn, int col, int argc, int serversPlayer){

    /* Input function to be called if a player chooses a row that is full. Displays an error 
       message and retakes the player's input.
       
       Params:
        - int turn - current turn
        - int col - the column that was attempted to be dropped into
        - int argc - argc to define server-mode and client-mode
        - int serversPlayer - Server's player (1 or 2) to determine whose current turn it is
         */

    char input;
    char * inp = &input;

    printf("\n\n");
    display();

    if(argc == 2)
    {
        /* Server-mode */

        if(turn == 1 && serversPlayer == 1)
        {
            /* Player 1's turn AND Server is Player 1 */

            printf("\nInvalid choice: Column %d is full\n", col);
            printf("Choose a different column\n");
            printf("\nTurn still: %s", player1);
            
            printf("\n\nEnter a letter A-G, for the column to drop a disc into...");
            printf("\nOr press 'Q' to quit the game.\n\n");

            scanf("%s", &input);

            while(input != 'A' && input != 'B' && input != 'C' && input != 'D' && input != 'E' 
            && input != 'F' && input != 'G' && input != 'Q')
            {
                printf("\nInvalid move.\nLetter entered must either be A-G or Q.\n\n");
                printf("Enter a letter, or press Q to quit.\n\n");
                scanf("%s", &input);
            }

            if(send(sockfd, inp, sizeof(inp), 0) == -1)
            {
                printf("\nError: inputRedo() > send() on Server-side failed.\n");
            }
        }
        else if(turn == 2 && serversPlayer == 2)
        {
            /* Player 2's turn AND Server is Player 2 */

            printf("\nInvalid choice: Column %d is full\n", col);
            printf("Choose a different column\n");
            printf("\nTurn still: %s", player2);

            printf("\n\nEnter a letter A-G, for the column to drop a disc into...");
            printf("\nOr press 'Q' to quit the game.\n\n");

            scanf("%s", &input);

            while(input != 'A' && input != 'B' && input != 'C' && input != 'D' && input != 'E' 
            && input != 'F' && input != 'G' && input != 'Q')
            {
                printf("\nInvalid move.\nLetter entered must either be A-G or Q.\n\n");
                printf("Enter a letter, or press Q to quit.\n\n");
                scanf("%s", &input);
            }

            if(send(sockfd, inp, sizeof(inp), 0) == -1)
            {
                printf("\nError: inputRedo() > send() on Server-side failed.\n");
            }
        }
        else if(turn == 1 && serversPlayer == 2 || turn == 2 && serversPlayer == 1)
        {
            /* It is not the Server-mode player's turn, regardless of whether it is the turn for player 1 or player 2 
            Note: turn == 1: player 1's turn    turn == 2: player 2's turn 
                    serversPlayer == 1: player 1      serversPlayer == 2: player 2 */

            printf("\nInvalid choice: Column %d is full\n", col);
            printf("Choose a different column\n");
            if(serversPlayer == 1)
            {
                printf("\nTurn still: %s - waiting for their move...\n\n\n", player2);
            }
            else if(serversPlayer == 2)
            {
                printf("\nTurn still: %s - waiting for their move...\n\n\n", player1);
            }

            if(recv(sockfd, inp, sizeof(inp), 0) == -1)
            {
                printf("\nError: inputRedo() > recv() on Server-side failed.\n");
            }
        }
    }

    if(argc == 3)
    {
        /* Client-mode */

        if(turn == 1 && serversPlayer == 2)
        {
            /* Player 1's turn AND Client is Player 1 */

            printf("\nInvalid choice: Column %d is full\n", col);
            printf("Choose a different column\n");
            printf("\nTurn still: %s", player1);

            printf("\n\nEnter a letter A-G, for the column to drop a disc into...");
            printf("\nOr press 'Q' to quit the game.\n\n");

            scanf("%s", &input);

            while(input != 'A' && input != 'B' && input != 'C' && input != 'D' && input != 'E' 
            && input != 'F' && input != 'G' && input != 'Q')
            {
                printf("\nInvalid move.\nLetter entered must either be A-G or Q.\n\n");
                printf("Enter a letter, or press Q to quit.\n\n");
                scanf("%s", &input);
            }

            if(send(sockfd, inp, sizeof(inp), 0) == -1)
            {
                printf("\nError: inputRedo() > send() on Client-side failed.\n");
            }
        }
        else if(turn == 2 && serversPlayer == 1)
        {
            /* Player 2's turn AND Client is Player 2 */

            printf("\nInvalid choice: Column %d is full\n", col);
            printf("Choose a different column\n");
            printf("\nTurn still: %s", player2);

            printf("\n\nEnter a letter A-G, for the column to drop a disc into...");
            printf("\nOr press 'Q' to quit the game.\n\n");

            scanf("%s", &input);

            while(input != 'A' && input != 'B' && input != 'C' && input != 'D' && input != 'E' 
            && input != 'F' && input != 'G' && input != 'Q')
            {
                printf("\nInvalid move.\nLetter entered must either be A-G or Q.\n\n");
                printf("Enter a letter, or press Q to quit.\n\n");
                scanf("%s", &input);
            }

            if(send(sockfd, inp, sizeof(inp), 0) == -1)
            {
                printf("\nError: inputRedo() > send() on Client-side failed.\n");
            }
        }
        else if(turn == 1 && serversPlayer == 1 || turn == 2 && serversPlayer == 2)
        {
            /* It is not the Client-mode player's turn, regardless of whether it is the turn for player 1 or player 2 
            Note: turn == 1: player 1's turn    turn == 2: player 2's turn 
                    serversPlayer == 1: player 1      serversPlayer == 2: player 2 */

            printf("\nInvalid choice: Column %d is full\n", col);
            printf("Choose a different column\n");
            if(serversPlayer == 1)
            {
                printf("\nTurn still: %s - waiting for their move...\n\n\n", player1);
            }
            else if(serversPlayer == 2)
            {
                printf("\nTurn still: %s - waiting for their move...\n\n\n", player2);
            }

            if(recv(sockfd, inp, sizeof(inp), 0) == -1)
            {
                printf("\nError: inputRedo() > recv() on Client-side failed.\n");
            }
        }
    }

    printf("Column chosen: %c\n", input);
    printf("----------------\n\n");

    return input;
}

