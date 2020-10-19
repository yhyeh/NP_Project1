#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>

using namespace std;

/* functions */
char** vecStrToChar(vector<string>);
bool hasPipe(vector<string>);
void printStrVec(vector<string>);
vector<vector<string>> splitPipe(vector<string>);
void purePipe(vector<string>);
int strToInt(string);
// vecetor<int> getPredecessor(int);

/* global vars */
int redirectFd;
// vector<string> lineHistory;
vector<int> outLinePfd;
vector<int> successor;
int iLine = -1;
vector<pid_t> childPids;
bool lsFlag;
bool pureFlag;
map<int, vector<int>> mapSuccessor;
map<int, vector<int>>::iterator mit;
bool sharePipeFlag;
bool pipeErrFlag;

/* macros */
#define PURE_PIPE 0
#define FILE_REDI 1
#define NUMB_PIPE 2

int main(int argc, char* const argv[], char *envp[]) {

  if(setenv("PATH", "bin:.", 1) == -1){
    cerr << "Error: set env err" << endl;
  }
  string wordInCmd;
  string cmdInLine;
  vector<string> cmd;

  while(true){
    wordInCmd.clear();
    cmd.clear();
    lsFlag = false;
    pureFlag = false;
    sharePipeFlag = false;
    pipeErrFlag = false;
    cout << "% ";
    // fflush(stdout);
    getline(cin, cmdInLine);
    // lineHistory.push_back(cmdInLine);
    iLine++; // for num pipe later
    outLinePfd.push_back(-1);
    successor.push_back(-1);
    
    // parse one line
    istringstream inCmd(cmdInLine);
    while (inCmd >> wordInCmd) {
      cmd.push_back(wordInCmd);
    }

    if (cmd.size() == 0){
      iLine--;
      outLinePfd.pop_back();
      successor.pop_back();
      continue;
    }else if (cmd[0] == "exit"){
      return 0;
    }else if (cmd[0] == "printenv"){
      if (cmd.size() == 2){
        if (getenv(cmd[1].c_str()) != NULL){
          cout << getenv(cmd[1].c_str()) << endl;
        }else{
          // cerr << "Error: no such env" << endl;
        }
      }else{
        cerr << "Error: missing argument" << endl;
      }
    }else if (cmd[0] == "setenv"){
      if (cmd.size() == 3){
        setenv(cmd[1].c_str(), cmd[2].c_str(), 1); // overwrite exist env
      }else{
        cerr << "Error: missing argument" << endl;
      }
    }else{ // non-buildin function
      
      // process each cmd seperate by > or |
      // Where is > or | ?
      string pipeMark = cmd[cmd.size()-1].substr(0, 1);
      if (pipeMark == "|" || pipeMark == "!"){ // |n or !n
        // cout << "This is |n" << endl;
        string afterMark = cmd[cmd.size()-1].substr(1);
        successor[iLine] = iLine + strToInt(afterMark);
        cmd.pop_back();
        mit = mapSuccessor.find(successor[iLine]);
        if (mit == mapSuccessor.end()){ // pass to new successor
          mapSuccessor[successor[iLine]] = vector<int>(2, -1);
        }else {
          sharePipeFlag = true;
        }
        if (pipeMark == "!") {
          pipeErrFlag = true;
          // cout << "This is !n" << endl;
        }
        purePipe(cmd);
        
        if (!sharePipeFlag){
          mapSuccessor[successor[iLine]][0] = outLinePfd[iLine];
        }else {
          close(outLinePfd[iLine]);
          // cout << "parent close [" << outLinePfd[iLine] << "]" << endl;
        }
      }
      /*else if (cmd[cmd.size()-1].substr(0, 1) == "!"){ // !n
        // cout << "This is !n" << endl;
        string afterMark = cmd[cmd.size()-1].substr(1);
        successor[iLine] = iLine + strToInt(afterMark);
        cmd.pop_back();

      }*/
      else if (cmd.size() > 1 && cmd[cmd.size()-2] == ">"){
        // cout << "This is >" << endl;
        string fname = cmd.back();
        // remove string after >
        cmd.pop_back();
        cmd.pop_back();
        // process as purepipe
        purePipe(cmd);

        // fork printer, wait printer
        pid_t printerPid;
        if ((printerPid = fork()) < 0) {
          cerr << "Error: fork failed" << endl;
          close(outLinePfd[iLine]);
          exit(0);
        }
        if (printerPid == 0){ // child
          // redirectFd = open(fname.c_str(), O_RDWR | O_CREAT | O_CLOEXEC);
          ofstream redirectFile(fname);
          dup2(outLinePfd[iLine], STDIN_FILENO);
          /* print */
          string line;
          while (getline(cin, line)) {
            redirectFile << line << endl;
          }

          redirectFile.close();
          for (int fd = 3; fd <= outLinePfd[iLine]; fd++){
            close(fd);
          }
          exit(0);
        }else{ // parent
          waitpid(printerPid, NULL, 0);
          close(outLinePfd[iLine]);
          // cout << "parent close [" << outLinePfd[iLine] << "]" << endl;
        }
      }
      else { // 0 ~ n pipe 
        // cout << "This is pure pipe" << endl;
        pureFlag = true;
        purePipe(cmd);
        if (lsFlag == true) continue;
        // fork printer, wait printer
        pid_t printerPid;
        if ((printerPid = fork()) < 0) {
          cerr << "Error: fork failed" << endl;
          close(outLinePfd[iLine]);
          exit(0);
        }
        if (printerPid == 0){ // child
          dup2(outLinePfd[iLine], STDIN_FILENO);
          /* test printer with number
          vector<string> tmp;
          tmp.push_back("number");
          if (execvp(tmp[0].c_str(), vecStrToChar(tmp)) == -1){
            cerr << "Unknown command: [" << tmp[0] << "]." << endl;
            exit(0);
          }
          */
          string line;
          while (getline(cin, line)) {
            cout << line << endl;
          }
          for (int fd = 3; fd <= outLinePfd[iLine]; fd++){
            close(fd);
          }
          exit(0);
        }else{ // parent
          waitpid(printerPid, NULL, 0);
          close(outLinePfd[iLine]);
          // cout << "parent close [" << outLinePfd[iLine] << "]" << endl;
        }
      }
      
    }
  }

  return 0;
}

void purePipe(vector<string> cmd){ // fork and connect sereval worker, but not guarantee finish 
  vector<vector<string>> cmdVec = splitPipe(cmd);
  childPids.clear();
  pid_t pid;
  int pfd[2];
  int prevPipeOutput = -1;
  
  /* pure ls */
  if (pureFlag && cmdVec.size() == 1 && cmdVec[0][0] == "ls") { 
    lsFlag = true;
    if ((pid = fork()) < 0) {
      cerr << "Error: fork failed" << endl;
      exit(0);
    }
    if (pid == 0){ // child
      if (execvp(cmdVec[0][0].c_str(), vecStrToChar(cmdVec[0])) == -1){
        cerr << "Unknown command: [" << cmdVec[0][0] << "]." << endl;
        exit(0);
      } 
    }else{ // parent
      waitpid(pid, NULL, 0);
    }
    return;
  }

  // vector<int> predecessor = getPredecessor(iLine);  
  // 0 ~ n pipe
  for (int icmd = 0; icmd < cmdVec.size(); icmd++){
    vector<string> curCmd = cmdVec[icmd];
    if (pipe(pfd) == -1){
      cerr << "Error: pipe create fail" << endl;
      exit(0);
    }
    if ((pid = fork()) < 0) {
      cerr << "Error: fork failed" << endl;
      exit(0);
    }
    /* child process */
    if (pid == 0){
      
      if (icmd == 0){ // first cmd
        map<int, vector<int>>::iterator mit;
        mit = mapSuccessor.find(iLine);
        if (mit != mapSuccessor.end()){ // has predecessor
          prevPipeOutput = mapSuccessor[iLine][0];
          dup2(prevPipeOutput, STDIN_FILENO); // stdin from previous cmd
          close(prevPipeOutput);
          close(mapSuccessor[iLine][1]);
        }
        if(successor[iLine] != -1 && icmd == cmdVec.size()-1){
          // numpipe && last cmd
          if (sharePipeFlag){
            dup2(mapSuccessor[successor[iLine]][1], STDOUT_FILENO); // output to shared pipe
            if (pipeErrFlag){
              int tgCopy = dup(mapSuccessor[successor[iLine]][1]);
              dup2(tgCopy, STDERR_FILENO); // output to shared pipe
              close(tgCopy);
            }
            close(mapSuccessor[successor[iLine]][1]);
          }else {
            dup2(pfd[1], STDOUT_FILENO); // output to pipe
            if (pipeErrFlag){
              int tgCopy = dup(pfd[1]);
              dup2(tgCopy, STDERR_FILENO); // output to pipe
              close(tgCopy);
            }
          }
        }else {
          dup2(pfd[1], STDOUT_FILENO); // output to pipe
        }
        
      }else { // mid cmd
        dup2(prevPipeOutput, STDIN_FILENO); // stdin from previous cmd
        close(prevPipeOutput);
        if (successor[iLine] != -1 && icmd == cmdVec.size()-1){
          // |n or !n & last cmd
          if (sharePipeFlag){
            dup2(mapSuccessor[successor[iLine]][1], STDOUT_FILENO); // output to shared pipe
            if (pipeErrFlag){
              int tgCopy = dup(mapSuccessor[successor[iLine]][1]);
              dup2(tgCopy, STDERR_FILENO); // output to shared pipe
              close(tgCopy);
            }
            close(mapSuccessor[successor[iLine]][1]);
          }else {
            dup2(pfd[1], STDOUT_FILENO); // output to pipe
            if (pipeErrFlag){
              int tgCopy = dup(pfd[1]);
              dup2(tgCopy, STDERR_FILENO); // output to pipe
              close(tgCopy);
            }
          }
        }else {
          dup2(pfd[1], STDOUT_FILENO); // output to pipe
        }
      }
      for (int fd = 3; fd <= pfd[0]; fd++){
        if (fd != prevPipeOutput){
          close(fd);
        }
      }
      close(pfd[1]);

      if(execvp(curCmd[0].c_str(), vecStrToChar(curCmd)) == -1){
        cerr << "Unknown command: [" << curCmd[0] << "]." << endl;
        close(pfd[1]); // necessary?
        /*if (cmdType == FILE_REDI) {
          if(close(redirectFd) < 0) {
            cerr << "Error: cannot close file @2"<< endl;
          }
        }*/
        exit(0);
      }
    } /* parent process */
    else {
      // cout << "create pfd: " << pfd[0] << " " << pfd[1] << endl; 
      if (successor[iLine] != -1 && !sharePipeFlag && icmd == cmdVec.size()-1){
        // numpipe && pass to new successor && last cmd
        mapSuccessor[successor[iLine]][1] = pfd[1];
      }else {
        close(pfd[1]);
        // cout << "parent close [" << pfd[1] << "]" << endl;
      }
      childPids.push_back(pid);
      if (icmd != 0){
        close(prevPipeOutput);
      }
      prevPipeOutput = pfd[0];
    }
  }
  outLinePfd[iLine] = pfd[0];
  mit = mapSuccessor.find(iLine);
  if (mit != mapSuccessor.end()){
    close(mapSuccessor[iLine][0]);
    close(mapSuccessor[iLine][1]);
    // cout << "parent close [" << mapSuccessor[iLine][0] << "]" << endl;
    // cout << "parent close [" << mapSuccessor[iLine][1] << "]" << endl;
  }
  /*
  if (successor[iLine] != -1 && sharePipeFlag){
    outLinePfd[iLine] = mapSuccessor[successor[iLine]][0];
  }else {
    outLinePfd[iLine] = pfd[0];
  }
  */
  /* wait child processes finished *//*
  for(int iproc = 0; iproc < childPids.size(); iproc++){
    waitpid(childPids[iproc], NULL, 0);
    // cout << "child " << iproc << " finished" << endl;
  }*/
  // close(pfd[0]);
}
/*
vecetor<int> getPredecessor(int iLine){
  vector<int> predecessor;
  for (int i=0; i < successor.size(); i++){
    if (successor[i] == iLine){
      predecessor.push_back(i);
    }
  }
  return predecessor;
}
*/
char** vecStrToChar(vector<string> cmd){
  char** result = (char**)malloc(sizeof(char*)*(cmd.size()+1));
  for(int i = 0; i < cmd.size(); i++){
    result[i] = strdup(cmd[i].c_str());
  }
  result[cmd.size()] = NULL;
  return result;
}

bool hasPipe(vector<string> cmd){
  bool flag = false;
  for (int i = 0; i < cmd.size(); i++){
    if (cmd[i] == "|"){
      flag = true;
      break;
    }
  }
  return flag;
}

void printStrVec(vector<string> v){ // just for testing
  cout << "strvec printer =========" << endl;
  for (int i = 0; i < v.size(); i++){
    cout << v[i] << endl;
  }
  cout << "========================" << endl;
  return;
}

vector<vector<string>> splitPipe(vector<string> cmd){
  vector<vector<string>> cmdVec;
  vector<string>::iterator ibeg = cmd.begin();
  for (vector<string>::iterator icur = cmd.begin(); icur != cmd.end(); icur++){
    if (*icur == "|"){
      vector<string> beforePipe(ibeg, icur);
      //printStrVec(beforePipe);
      cmdVec.push_back(beforePipe);
      ibeg = icur + 1;
    }else if (icur == cmd.end()-1){
      vector<string> afterPipe(ibeg, cmd.end());
      //printStrVec(afterPipe);
      cmdVec.push_back(afterPipe);
    }
  }
  return cmdVec;
}

int strToInt(string str){
  return atoi(str.c_str());
}
