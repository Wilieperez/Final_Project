#include "MQTT_frames.h"

pthread_mutex_t mutex;
r_Client clients[BACKLOG];
fd_set read_fds;
int max_fd;

f_PingRequest reqPing_frame;
f_PingResponse resPing_frame;

f_ConnAcknowledge create_ConnAck(uint8_t ret_code){
    f_ConnAcknowledge frame;

    frame.bFrameType = 0x20;
    frame.bRemainLen = 0x02;
    frame.bReservedVal = 0x00;
    frame.bReturnCode = ret_code;
   
    return frame;
}

f_PingResponse create_PRes(){
    f_PingResponse frame;

    frame.bFrameType = 0xD0;
    frame.bresponse = 0x00;

    return frame;
}

f_SubAcknowledge create_SubAck(uint8_t response){
    f_SubAcknowledge frame;

    frame.bFrameType = 0x90;
    frame.bRemainLen = 0x01;
    frame.bResponse = response;

    return frame;
}

f_UnsubAcknowledge create_UnsubAck(uint8_t response){
    f_UnsubAcknowledge frame;

    frame.bFrameType = 0xB0;
    frame.bRemainLen = 0x01;
    frame.bResponse = response;

    return frame;
}

f_PubAcknowledge create_PubAck(){
    f_PubAcknowledge frame;

    frame.bFrameType = 0x40;
    frame.bRemainLen = 0x00;

    return frame;
}

void timer_handler (int signum)
{
   for(int f = 0; f < BACKLOG; f++){
      if(clients[f].fd != 0){
         clients[f].iKeepAlive = clients[f].iKeepAlive - 0x01;//Subtract 1
         //printf("%s ka: %i\n",clients[f].sUsername,clients[f].iKeepAlive);
         if(clients[f].iKeepAlive <= 0){//Client keep alive gets to 0
            printf("%s disconnected\n", clients[f].sUsername);
            if(pthread_mutex_lock(&mutex) == 0){
               close(clients[f].fd);
               FD_CLR(clients[f].fd, &read_fds);
               clients[f].fd = 0;//Removes that clients file descriptor from list
               pthread_mutex_unlock(&mutex);
            }
         }
      }
   }//Subtracts 1 from every keep alive that has a file descriptor
}

void *timer_count(void *param){
   //Timer Variables
   struct sigaction sa;
	struct itimerval timer;

   //Install timer_handler as the signal handler for SIGVTALRM.
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &timer_handler;
	sigaction (SIGVTALRM, &sa, NULL);
	
	//Configure the timer to expire after seconds
	timer.it_value.tv_sec = 1;
	timer.it_value.tv_usec = 0;
	
	//and every certain seconds after that
	timer.it_interval.tv_sec = 1;
	timer.it_interval.tv_usec = 0;

   setitimer(ITIMER_VIRTUAL, &timer, NULL);//Timer start

   while(1);//Goes into infinite cycle so the timer can go off
}

void *handle_client(void *param) {
   int read_size;
   f_Publish pub_frame;
   f_PubAcknowledge pubAck_frame;
   f_Subscribe sub_frame;
   f_SubAcknowledge subAck_frame;
   f_Unsubscribe unsub_frame;
   f_UnsubAcknowledge unsubAck_frame;

   pubAck_frame = create_PubAck();//Create default publish acknowledge frame

   if(pthread_mutex_lock(&mutex) == 0){
      if ((read_size = select(max_fd + 1, &read_fds, NULL, NULL, NULL)) == -1) {
         printf("select failed\n");
      }//Starts monitoring file descriptors
      pthread_mutex_unlock(&mutex);
   }

   while(1){
      for(int i = 0; i < BACKLOG; i++){
         if(FD_ISSET(clients[i].fd, &read_fds)){//If a fd is ready
            if((read_size = recv(clients[i].fd ,(char *)&pub_frame , sizeof(f_Publish),0)) == -1){
               printf("failed to recieve");
               continue;
            }//Read client frame

            if(read_size <= 0){//Client disconnected
               printf("%s disconnected\n", clients[i].sUsername);
               if(pthread_mutex_lock(&mutex) == 0){
                  close(clients[i].fd);
                  FD_CLR(clients[i].fd, &read_fds);
                  clients[i].fd = 0;//Removes that clients file descriptor from list
                  pthread_mutex_unlock(&mutex);
               }
            }
            if(pub_frame.bFrameType == 0xC0){//Ping Request frame type
               reqPing_frame.bFrameType = pub_frame.bFrameType;
               reqPing_frame.bkeepAlive = pub_frame.bRemainLen;
               clients[i].iKeepAlive = clients[i].iKeepAliveMax;
               if(clients[i].fd != 0){
                  send(clients[i].fd, &resPing_frame, sizeof(f_PingResponse), 0);
               }

            }else if(pub_frame.bFrameType == 0x80){//Subscribe frame type
               sub_frame.bFrameType = pub_frame.bFrameType;
               sub_frame.bRemainLen = pub_frame.bRemainLen;
               sub_frame.bTopic = pub_frame.bTopic;
               if(sub_frame.bTopic == 0x00 && clients[i].bTemperature == false){
                  clients[i].bTemperature = true;
                  printf("%s subscribed to Temperature\n", clients[i].sUsername);
                  subAck_frame = create_SubAck(0x00);
               }else if(sub_frame.bTopic == 0x01 && clients[i].bRainChance == false){
                  clients[i].bRainChance = true;
                  printf("%s subscribed to Rain Chance\n", clients[i].sUsername);
                  subAck_frame = create_SubAck(0x00);
               }else if(sub_frame.bTopic == 0x02 && clients[i].bHumidity == false){
                  clients[i].bHumidity = true;
                  printf("%s subscribed to Humidity\n", clients[i].sUsername);
                  subAck_frame = create_SubAck(0x00);
               }else if(sub_frame.bTopic == 0x03 && clients[i].bUVIndex == false){
                  clients[i].bUVIndex = true;
                  printf("%s subscribed to UV Index\n", clients[i].sUsername);
                  subAck_frame = create_SubAck(0x00);
               }else if(sub_frame.bTopic == 0x04 && clients[i].bAirQuality == false){
                  clients[i].bAirQuality = true;
                  printf("%s subscribed to Air Quality\n", clients[i].sUsername);
                  subAck_frame = create_SubAck(0x00);
               }else{
                  subAck_frame = create_SubAck(0x01);
               }
               if(clients[i].fd != 0){
                  send(clients[i].fd, &subAck_frame, sizeof(f_SubAcknowledge), 0);
               }

            }else if(pub_frame.bFrameType == 0x30){//Publish frame type 
               if(pub_frame.bTopic == 0x00){
                  printf("%s published to Temperature\n", clients[i].sUsername);
                  for(int j = 0; j < BACKLOG; j++){
                     if(clients[j].fd != 0 && clients[j].bTemperature == true && i != j){
                        send(clients[j].fd, &pub_frame, sizeof(f_Publish), 0);
                     }//Filters clients that are subscribed to Temperature, excluding the sender
                  }
               }else if(pub_frame.bTopic == 0x01){
                  printf("%s published to Rain Chance\n", clients[i].sUsername);
                  for(int j = 0; j < BACKLOG; j++){
                     if(clients[j].fd != 0 && clients[j].bRainChance == true && i != j){
                        send(clients[j].fd, &pub_frame, sizeof(f_Publish), 0);
                     }//Filters clients that are subscribed to Rain chance, excluding the sender
                  }
               }else if(pub_frame.bTopic == 0x02){
                  printf("%s published to Humidity\n", clients[i].sUsername);
                  for(int j = 0; j < BACKLOG; j++){
                     if(clients[j].fd != 0 && clients[j].bHumidity == true && i != j){
                        send(clients[j].fd, &pub_frame, sizeof(f_Publish), 0);
                     }//Filters clients that are subscribed to Humidity, excluding the sender
                  }
               }else if(pub_frame.bTopic == 0x03){
                  printf("%s published to UV Index\n", clients[i].sUsername);
                  for(int j = 0; j < BACKLOG; j++){
                     if(clients[j].fd != 0 && clients[j].bUVIndex == true && i != j){
                        send(clients[j].fd, &pub_frame, sizeof(f_Publish), 0);
                     }//Filters clients that are subscribed to Rain chance, excluding the sender
                  }
               }else if(pub_frame.bTopic == 0x04){
                  printf("%s published to Air Quality\n", clients[i].sUsername);
                  for(int j = 0; j < BACKLOG; j++){
                     if(clients[j].fd != 0 && clients[j].bAirQuality == true && i != j){
                        send(clients[j].fd, &pub_frame, sizeof(f_Publish), 0);
                     }//Filters clients that are subscribed to Rain chance, excluding the sender
                  }
               }
               if(clients[i].fd != 0){
                  send(clients[i].fd, &pubAck_frame, sizeof(f_PubAcknowledge), 0);
               }

            }else if(pub_frame.bFrameType == 0xA0){//Unsubscribe frame type
               unsub_frame.bFrameType = pub_frame.bFrameType;
               unsub_frame.bRemainLen = pub_frame.bRemainLen;
               unsub_frame.bTopic = pub_frame.bTopic;
               if(unsub_frame.bTopic == 0x00 && clients[i].bTemperature == true){
                  clients[i].bTemperature = false;
                  printf("%s unsubscribed to Temperature\n", clients[i].sUsername);
                  unsubAck_frame = create_UnsubAck(0x00);
               }else if(unsub_frame.bTopic == 0x01 && clients[i].bRainChance == true){
                  clients[i].bRainChance = false;
                  printf("%s unsubscribed to Rain Chance\n", clients[i].sUsername);
                  unsubAck_frame = create_UnsubAck(0x00);
               }else if(unsub_frame.bTopic == 0x02 && clients[i].bHumidity == true){
                  clients[i].bHumidity = false;
                  printf("%s unsubscribed to Humidity\n", clients[i].sUsername);
                  unsubAck_frame = create_UnsubAck(0x00);
               }else if(unsub_frame.bTopic == 0x03 && clients[i].bUVIndex == true){
                  clients[i].bUVIndex = false;
                  printf("%s unsubscribed to UV Index\n", clients[i].sUsername);
                  unsubAck_frame = create_UnsubAck(0x00);
               }else if(unsub_frame.bTopic == 0x04 && clients[i].bAirQuality == true){
                  clients[i].bAirQuality = false;
                  printf("%s unsubscribed to Air Quality\n", clients[i].sUsername);
                  unsubAck_frame = create_UnsubAck(0x00);
               }else{
                  unsubAck_frame = create_UnsubAck(0x01);
               }
               if(clients[i].fd != 0){
                  send(clients[i].fd, &unsubAck_frame, sizeof(f_UnsubAcknowledge), 0);
               }
            }
         }
      }
   }

   if(close(clients[0].fd) < 0) {
      perror("Close socket failed\n");
   }

   pthread_exit(NULL);
}

int main(int argc, char **argv) {
   int sockfd, newfd, numbytes;
   struct sockaddr_in host_addr, client_addr;
   pthread_t thread,thread2;
   f_Connect conn_frame;
   f_ConnAcknowledge connack_frame;
   socklen_t sin_size;
   resPing_frame = create_PRes();

   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("Socket failed");
      exit(-1);
   }
   puts("--Socket created\n");

   host_addr.sin_family = AF_INET;
   host_addr.sin_port = htons(PORT);
   host_addr.sin_addr.s_addr = INADDR_ANY;
   memset(&(host_addr.sin_zero), '\0', 8);

   if(bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1) {
      perror("Bind failed");
      exit(-1);
   }
   puts("--Address binded\n");

   if(listen(sockfd, BACKLOG) == -1) {
      perror("Listen failed");
      exit(-1);
   }
   puts("--Listening...\n");

   sin_size = sizeof(struct sockaddr_in);

   // initialize the fd_set with the server socket
   FD_ZERO(&read_fds);
   FD_SET(sockfd, &read_fds);
   max_fd = sockfd;

   if(pthread_mutex_init (&mutex, NULL) != 0){
      printf("Failed to initialize mutex");
   }//Initialize mutex variables
   
   if(pthread_create(&thread2, NULL, timer_count, NULL) < 0) {
      perror("Thread creation failed");
      exit(-1);
   }

   if(pthread_create(&thread, NULL, handle_client, NULL) < 0) {
      perror("Thread creation failed");
      exit(-1);
   }

   while(1){
      if((newfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size)) > 0){  
         if ((numbytes=recv(newfd,(char *)&conn_frame,sizeof(f_Connect),0)) == -1){
            printf("failed to recieve\n");
         }
         if(conn_frame.bFrameType == 0x10){//================Add more parameters to assure its correct
            for(int i = 0; i < BACKLOG; i++){
               if(clients[i].fd == 0){//If register spot is empty, fills out new client info
                  if(pthread_mutex_lock(&mutex) == 0){
                     clients[i].fd = newfd;
                     strcpy(clients[i].sUsername, conn_frame.sClientID);
                     clients[i].iKeepAlive = conn_frame.bKeepAlive;
                     clients[i].iKeepAliveMax = conn_frame.bKeepAlive;
                     clients[i].bTemperature = false;
                     clients[i].bRainChance = false;
                     clients[i].bHumidity = false;
                     clients[i].bUVIndex = false;
                     clients[i].bAirQuality = false;
                     printf("New client found!\n\n");
                     FD_SET(clients[i].fd, &read_fds);
                     if (clients[i].fd > max_fd) {
                        max_fd = clients[i].fd;
                     }//Sets the new max fd for the select function
                     connack_frame = create_ConnAck(0x00);
                     if(send(newfd, &connack_frame, sizeof(f_ConnAcknowledge), 0) < 0) {
                        perror("Send failed\n");
                     }//Creates connack frame and sends it to client
                     pthread_mutex_unlock(&mutex);
                  }
                  break;
               }
               if(i == BACKLOG-1 && clients[i].fd != 0){//If program reaches the last register and its not empty
                  printf("Client list full\n\n"); 
                  connack_frame = create_ConnAck(0x03);
                  if(send(newfd, &connack_frame, sizeof(f_ConnAcknowledge), 0) < 0) {
                     perror("Send failed\n");
                  }//Creates connack frame with error code and sends it to client
               }
            }
         }else{
            connack_frame = create_ConnAck(0x01);//If frame sent by client is not connect frame
            if(send(newfd, &connack_frame, sizeof(f_ConnAcknowledge), 0) < 0) {
               perror("Send failed\n");
            }//Creates connack frame with error code and sends it to client
         }
      }
   }

   pthread_join(thread, NULL);
   pthread_join(thread2, NULL);

      if(close(sockfd) < 0) {
      perror("Close socket failed\n");
      exit(-1);
   }

   return 0;
}