#include <iostream>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <assert.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#define USER_LIMIT 5
#define BUFFER_SIZE 64
#define FD_LIMIT 65535
using namespace std;
struct client_date{
    sockaddr_in address;
    char* write_buff;
    char buf[BUFFER_SIZE];
};
int setnonblocking(int fd){
    int old_option= fcntl(fd,F_GETFL);
    int new_option=old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}
int main() {
    int sersock= socket(AF_INET,SOCK_STREAM,0);
    assert(sersock>=0);
    struct sockaddr_in seraddr;
    memset(&seraddr,0,sizeof(sockaddr_in));
    seraddr.sin_port= htons(8090);
    seraddr.sin_family=AF_INET;
    seraddr.sin_addr.s_addr= htonl(INADDR_ANY);
    if(bind(sersock,(sockaddr *) &seraddr,sizeof(seraddr))<0){
        cout << "sock bind error" <<endl;
        return -2;
    }
    int ret= listen(sersock,5);
    int user_count=0;
    assert(ret!=-1);
    client_date *users=new client_date[FD_LIMIT];
    pollfd fds[USER_LIMIT+1];
    for(int i=1;i<=USER_LIMIT;i++){
        fds[i].events=0;
        fds[i].fd=-1;
    }
    fds[0].fd=sersock;
    fds[0].revents=0;
    fds[0].events=POLLIN|POLLERR;
    while(1){
        ret=poll(fds,user_count+1,-1);
        if(ret<0){
            cout << "poll error" <<endl;
            break;
        }
        for(int i=0;i<user_count+1;i++){
            if((fds[i].fd== sersock)&&(fds[i].revents&POLLIN)){
                struct sockaddr_in cliaddr;
                socklen_t client_addrlen=sizeof(cliaddr);
                int connfd= accept(sersock,(struct sockaddr *)&cliaddr,&client_addrlen);
                if(connfd<0){
                    cout <<"error occur"<<endl;
                    continue;
                }
                if(user_count>=USER_LIMIT){
                    char *buf="too many user";
                    cout << "too many user"<<endl;
                    send(connfd,buf,sizeof (buf),0);
                    close(connfd);
                    continue;
                }
                user_count++;
                users[connfd].address=cliaddr;
                setnonblocking(connfd);
                fds[user_count].fd=connfd;
                fds[user_count].events=POLLIN|POLLRDHUP|POLLERR;
                fds[user_count].revents=0;
                cout << "new user connect now have "<<user_count<<" users"<<endl;
            }else if(fds[i].revents&POLLERR){
                cout << "get a error"<<endl;
                char errors[100];
                memset(errors,'\0',100);
                socklen_t length=sizeof(errors);
                if(getsockopt(fds[i].fd,SOL_SOCKET,SO_ERROR,&errors,&length)<0){
                    cout <<"get socket option failer"<<endl;
                }
                continue;
            }else if(fds[i].revents&POLLRDHUP){
                users[fds[i].fd]=users[fds[user_count].fd];
                close(fds[i].fd);
                fds[i]=fds[user_count];
                user_count--;
                i--;
                cout <<  "a client leave"<<endl;
                cout << "now have "<<user_count<<" users"<<endl;
            }else if(fds[i].revents&POLLIN){
                int connfd=fds[i].fd;
                memset(users[connfd].buf,'\0',BUFFER_SIZE);
                ret = recv(connfd,users[connfd].buf,BUFFER_SIZE-1,0);
                cout << "get "<<ret<<" size buffer"<<endl;
                if(ret<0){
                    if(errno!=EAGAIN){
                        close(connfd);
                        users[fds[i].fd]=users[fds[user_count].fd];
                        fds[i]=fds[user_count];
                        i--;
                        user_count--;
                    }
                }else if(ret ==0){

                }else{
                    for(int j=1;j<user_count+1;j++){
                        if(fds[i].fd=connfd){
                            continue;
                        }
                        fds[j].events|=~POLLIN;
                        fds[j].events|=POLLOUT;
                        users[fds[i].fd].write_buff=users[connfd].buf;
                    }
                }
            }else if(fds[i].revents&POLLOUT){
                int connfd=fds[i].fd;
                if(!users[connfd].write_buff){
                    continue;
                }
                ret =send(connfd,users[connfd].buf,sizeof(users[connfd].buf),0);
                users[connfd].write_buff=NULL;
                fds[i].events|=~POLLOUT;
                fds[i].events|=POLLIN;
            }
        }
    }
    delete[] users;
    close(sersock);
    return 0;
}
