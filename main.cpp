
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <semaphore.h>
using namespace std;




vector<int> seats(100,-1);  //vector of the actual situation of the seats.The logs will be to this vector.
                            //seats[a]=-1 implies seat a+1 is empty.seats[a]=50 implies seat a+1 is reserved by client 51
                            //the reason the difference of 1 is that client numbers begin with 1 but i used vectors
                            // to begin from index 0.

vector<int> seatcopy(100,-1);//This is used to store the chosen(not reserved) seats so that no other client
                             // chooses the same seat.seatcopy[a]=-1 implies seat a+1 is not chosen by any other client.


vector<int> chosenseat(100,-1);  //this vector supplies communication between client and server threads.
                                //chosenseat[a] shows the seat number of the seat client(a+1) has chosen.
                                //Server of this client will read chosenseat[a] and reserve it for the client
                                //chosenseat[a]=30 tells the server thread of client a+1 that client a+1 chose the seat31.

pthread_mutex_t t1=PTHREAD_MUTEX_INITIALIZER;   //mutex lock to block other threads


//following vector are used for communication between a client and its server thread.
//This means c1[a] is only used by client thread or server thread of client(a+1)

vector<sem_t> c1(100);


ofstream outfile("output.txt");



void *server(void *param);  //function on which server threads will run
void *client(void *param);  //function on which client htreads will run


int main(int argc, char *argv[]) {





    if (argc != 2) {
        fprintf(stderr, "usage: a.out <integer value between 50 and 100>\n");
        return -1;
    }
    int arg = atoi(argv[1]);    //number of seats given in parameter

    if (arg < 50 || arg > 100) { // if the value that user enters smaller than 2
        fprintf(stderr, "Argument must be between 50 and 100 \n");
        return -1;
    }

    pthread_t clients[arg];
    pthread_t servers[arg];

    for (int k = 0; k < 100; k++) {     //initialize semaphores
        sem_init(&c1[k],0,0);
    }

    seats=vector<int> (seats.begin(),seats.begin()+arg);
    int ids[arg];
    outfile<<"Number of total seats: "<<arg<<endl;
    for (int k = 0; k < arg; k++) {
        ids[k]=k;

        pthread_create(&clients[k], 0, client, (void*)(ids+k)); //create client threads,give them the parameter(client number -1)

        pthread_create(&servers[k], 0, server, (void*)(ids+k));//create server threads,give them the parameter(client number -1)
    }

    for (int k = 0; k < arg; k++) {     //join all threads to main thread

        pthread_join(clients[k], NULL);
        pthread_join(servers[k], NULL);

    }

    outfile<<"All seats are reserved.";

}



void *server(void *param){



        int a=*(int*) param;    //get the client number this server is related with(It is 0 for Client1;1 for client2 ...)






        //server thread must wait here until its client sends a seat number it chose
        sem_wait(&c1[a]);


        int chosen=chosenseat[a];


        pthread_mutex_lock(&t1);    //other threads should be blocked because the reserved seats array changes here.
                                    //And seats should not be reserved at the same time ,otherwise printing the logs
                                    //will spoil.

            seats[chosen] = a;  //log the seat chosen+1 to client a+1
            outfile<<"Client"<<a+1<<" reserves "<<"Seat"<<(chosen+1)<<endl; //print the log
        pthread_mutex_unlock(&t1);

        sem_destroy(&c1[a]);
pthread_exit(0);
}




void *client(void *param){

        srand(time(NULL));
        int waittime=rand()%151+50; //select a random number between 50 and 200
        usleep(waittime);   //sleep for 50-200 ms

        int a=*(int*) param;   //get the client number(It is 0 for Client1;1 for client2 ...)



        vector<int> emptyseat;      //list of the seats not reserved yet

        pthread_mutex_lock(&t1);       //all other clients will wait until this client chooses a seat
                                        //so that empty seats or not-chosen seats won't change while this client
                                        //chooses a seat

        for (int k = 0; k < seats.size(); k++) {    //save seat numbers of the seats that are not reserved yet
            if (seats[k] == -1) {
                emptyseat.push_back(k);
            }
        }



        bool val = false;
        int seatnum;

        while (!val) {      //select a random seat from empty seats until we find a seat no other client has chosen
            int randnum =rand() %(int) emptyseat.size();
            seatnum = emptyseat[randnum];

            if (seatcopy[seatnum] == -1) {
                seatcopy[seatnum] = a;
                val=true;
            }


        }

        pthread_mutex_unlock(&t1);   //release the lock so that other clients can choose their seats and server threads
                                    //can make reservations


    chosenseat[a]=seatnum;   //send the chosen seat to server thread

    //Signal server thread that the client sent a seat number
        sem_post(&c1[a]);




pthread_exit(0);

}
