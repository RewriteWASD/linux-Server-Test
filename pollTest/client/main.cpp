#include <iostream>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
# define BUFF_MAXSIZE 64
using namespace std;
int main() {
    int clisock= socket(AF_INET,SOCK_STREAM,0);
    assert(clisock>=0);
    struct sockaddr_in sersock;
    memset(&sersock,0,sizeof(sockaddr_in));

    sersock.sin_addr.s_addr= htonl(INADDR_ANY);
    sersock.sin_port= htons(8090);
    sersock.sin_family=AF_INET;
    if(connect(clisock,(sockaddr*) &sersock,sizeof(sersock))<0){
        cout <<"connect error occur"<<endl;
        return 0;
    }
    char* buffl="hello server";
    cout <<buffl<<endl;
    if(send(clisock,buffl,sizeof(buffl),0)<0){
        cout << "send error"<<endl;
    }
    pollfd fds[2];
    fds[0].fd=0;
    fds[1].fd=clisock;
    fds[0].events=POLLIN;
    fds[1].events=POLLIN|POLLRDHUP;
    fds[0].revents=0;
    fds[1].revents=0;
    char read_buf[BUFF_MAXSIZE];
    int pipefd[2];
    int ret=pipe(pipefd);
    assert(ret!=-1);
    while(1){
        ret=poll(fds,2,-1);
        assert(ret>=0);
        if(fds[1].revents&POLLRDHUP){
            memset(read_buf,0,64);
            recv(fds[1].fd,read_buf,BUFF_MAXSIZE-1,0);
            cout << read_buf<<endl;
            cout<<"server close connection"<<endl;

            break;
        }else if(fds[1].revents&POLLIN){
            memset(read_buf,0,64);
            recv(fds[1].fd,read_buf,BUFF_MAXSIZE-1,0);
            cout << read_buf<<endl;
        }
        if(fds[0].revents&POLLIN){
            ret=splice(0,NULL,pipefd[1],NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
            ret=splice(0,NULL,clisock,NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
        }
    }
    close(clisock);
    return 0;
}
