#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <string>
#include <cstring>
#include <vector>

using namespace std;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* thread_func(void* arg) {

    //LOCK THE CRITICAL SECTION
    pthread_mutex_lock (&lock);

    cout<<"---- "<< pthread_self() <<endl;

    int fd = *(int*)arg;    //get passed pipe end from arg
    FILE *pipedContent = fdopen(fd, "r");   //open piped content for reading

    char buf[610]; 

    while ( fgets( buf, sizeof(buf),pipedContent )) {   //while fgets doesnt return null keep getting piped content into buf array
        printf("%s", buf);  //print buf array,
    }

    cout<<"---- "<< pthread_self() <<endl;

    pthread_mutex_unlock (&lock);
    //END OF THE CRITICAL SECTION, UNLCOK

    return NULL;
}

int main() {

    vector<int> p_id; //LIST OF PROCESS ID
    vector<pthread_t> t_id; //LIST OF THREAD

    //clean parse.txt in each run
    ofstream output("parse.txt");
    output.close();

    ifstream file("commands.txt");
    string line;

    //read each command line by line
    while ( getline(file, line) ) {

        string word;    
        istringstream iss(line);
        iss >> word;

        string parseInfo[6]; // indexes and values: 0 command, 1 inputs, 2 options, 3 redirection, 4 background job, 5 redirected filename
        parseInfo[0] = word;  //first word is always the command
        parseInfo[1] = "";
        parseInfo[2] = "";
        parseInfo[3] = "-";
        parseInfo[4] = "n";

        int index = 1;

        while ( iss >> word ) { //PARSING COMMAND

            bool wordCheck = true; //while true word is still not added to the parse arry so keep checking indexes

            if ( index == 1 && wordCheck ) {

                index++;
                if ( word[0] != '<' && word[0] != '>' && word[0] != '&' && word[0] != '-' ) {
                    wordCheck = false;
                    parseInfo[1] = word;
                } 
            }
            if ( index == 2 && wordCheck ) {

                index++;
                if ( word[0] == '-' ) {
                    wordCheck = false;
                    parseInfo[2] = word;
                } 
            }
            if( index == 3 && wordCheck) {
                
                index++;
                if(word[0] == '<' || word[0] == '>')  {
                    wordCheck = false;
                    parseInfo[3] = word[0];
                    iss >> word;
                    parseInfo[5] = word;
                } 
            }
            if( index == 4 && wordCheck ) {

                index++;
                if( word == "&") {
                    wordCheck = false;
                    parseInfo[4] = "y";
                }    
            }
        }

        ofstream output("parse.txt", ios::app);
        output << "----------"<<endl;
        output << "Command: " << parseInfo[0] <<endl;
        output << "Inputs: " << parseInfo[1] <<endl;
        output << "Options: " << parseInfo[2] <<endl;
        output << "Redirection: " << parseInfo[3] <<endl;
        output << "Background Job: " << parseInfo[4] <<endl;
        output << "----------"<<endl;
        output.close();

        bool redirection = false;
        if(parseInfo[3] == ">")
            redirection = true;

        if ( parseInfo[0] == "wait") {  //special wait command is issued

            for (int i = 0; i< p_id.size(); i++) {
                    
                int info;
                waitpid( p_id[i], &info, 0); 
            }
            for (int i = 0; i<t_id.size(); i++ ) {

                pthread_join(t_id[i], NULL);
            }

            //ALL PROCESSES AND THREADS ARE DONE SO BOTH LISTS SHOULD BE EMPTIED
            p_id.clear();
            t_id.clear();
        }
        else {

            if( redirection ) { //OUTPUT REDIRECTION EXIST
            
                int cp = fork();

                if (cp == 0) {   //COMMAND PROCESS 

                    close (STDOUT_FILENO);
                    open(parseInfo[5].c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

                    int range = 2;
                    if (parseInfo[1] != "") {
                        range++;
                    }
                    if(parseInfo[2] != ""){
                        range++;
                    }

                    char *myargs[range];
                    myargs[0] = strdup(parseInfo[0].c_str());
                    index = 1;

                    for (int i = 0; i<2; i++) { //check if inputs and options exist. If so also execute them.

                        if( parseInfo[1+i] != "") {
                            
                            myargs[index] = strdup(parseInfo[1+i].c_str());
                            index++;
                        }
                    }
                    myargs[index] = NULL;
                    execvp(myargs[0], myargs);

                }
                else if (cp > 0 ) {
                    
                    p_id.push_back(cp);
                    if ( parseInfo[4] == "n") { //SHELL PROCESS

                        p_id.pop_back();
                        int info;
                        waitpid( cp, &info, 0);   //if there is no & wait for child to finish
                    }
                }
            }
            else {  //NO OUTPUT REDIRECTION

                int *fd = new int[2]; 
                pipe(fd);
                pthread_t thread;

                int cp = fork();

                if (cp == 0) {   //COMMAND PROCESS 

                    close(fd[0]);
                    dup2(fd[1], STDOUT_FILENO);

                    if ( parseInfo[3] == "<" ) {  //input redirectioning if exist

                        close(STDIN_FILENO);
                        open( parseInfo[5].c_str(), O_RDONLY, S_IRWXU );
                    }

                    int range = 2;
                    if (parseInfo[1] != "") {
                        range++;
                    }
                    if(parseInfo[2] != ""){
                        range++;
                    }

                    char *myargs[range];
                    myargs[0] = strdup(parseInfo[0].c_str());
                    index = 1;

                    for (int i = 0; i<2; i++) { //check if inputs and options exist. If so also execute them.

                        if( parseInfo[1+i] != "") {
                            
                            myargs[index] = strdup(parseInfo[1+i].c_str());
                            index++;
                        }
                    }

                    myargs[index] = NULL;
                    execvp(myargs[0], myargs);

                }
                else if (cp > 0 ) { //SHELL PROCESS

                    p_id.push_back(cp);
                    close(fd[1]);
                    pthread_create(&thread, NULL, thread_func, &fd[0]);
                    t_id.push_back(thread);

                    if (parseInfo[4] == "n") {
                        int info;
                        waitpid( cp, &info, 0);   //if there is no & wait for child to finish
                        pthread_join(thread, NULL);

                        t_id.pop_back();
                        p_id.pop_back();
                    }
                }

            }
        }
    }

    file.close();
    
    //WAIT FOR ALL PROCESSES AND THREADS TO FINISH SHELL EXECUTION

    for (int i = 0; i< p_id.size(); i++) {
            
        int info;
        waitpid( p_id[i], &info, 0); 
    }
    for (int i = 0; i<t_id.size(); i++ ) {

        pthread_join(t_id[i], NULL);
    }

    return 0;

    }