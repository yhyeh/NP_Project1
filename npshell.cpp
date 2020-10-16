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

using namespace std;

char** vecStrToChar(vector<string>);
bool hasPipe(vector<string>);
void printStrVec(vector<string>);
vector<vector<string>> splitPipe(vector<string>);

int main(int argc, char* const argv[], char *envp[]) {

  if(setenv("PATH", "bin:.", 1) == -1){
    cerr << "Error: set env err" << endl;
  }
  string wordInCmd;
  string cmdInLine;
  vector<string> cmd;
  vector<string> cmdHistory;
  int iCmd = -1;

  while(true){
    wordInCmd.clear();
    cmd.clear();
    cout << "% ";
    // fflush(stdout);
    getline(cin, cmdInLine);
    cmdHistory.push_back(cmdInLine);
    iCmd++; // for num pipe
    
    // parse one line
    istringstream inCmd(cmdInLine);
    while (inCmd >> wordInCmd) {
      cmd.push_back(wordInCmd);
    }

    if (cmd.size() == 0){
      continue;
    }else if (cmd[0] == "exit"){
      return 0;
    }else if (cmd[0] == "printenv"){
      if (cmd.size() == 2){
        if (getenv(cmd[1].c_str()) != NULL){
          cout << getenv(cmd[1].c_str()) << endl;
        }else{
          cerr << "Error: no such env" << endl;
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
      
      if (cmd[cmd.size()-1].substr(0, 1) == "|"){ // |n?
        // string afterPipe = cmd[cmd.size()-1].substr(1);
        cout << "This is |n" << endl;
      }else if (cmd[cmd.size()-1].substr(0, 1) == "!"){ // !n
        cout << "This is !n" << endl;
      }else if (cmd.size() > 1 && cmd[cmd.size()-2] == ">"){
        cout << "This is >" << endl;
        string fname = cmd.back();
        // remove string after >
        // process as multi pipe
      }else if (hasPipe(cmd)){
        cout << "This is |" << endl;
        vector<vector<string>> cmdVec = splitPipe(cmd);
      }else{ // single cmd
        pid_t pid;
        // cout << "I will fork" << endl;
        pid = fork();
        if (pid == 0){ // child
          // string restOfCmd = cmdInLine.substr(cmd[0].size()+1);
          if(execvp(cmd[0].c_str(), vecStrToChar(cmd)) == -1){
            cerr << "Unknown command: [" << cmd[0] << "]" << endl;
          }
          
        }else{ // parent
          waitpid(pid, NULL, 0);
        }
      }
      
      
    }
  }

  return 0;
}

char** vecStrToChar(vector<string> cmd){
  char** result = (char**)malloc(sizeof(char*)*(cmd.size()+1));
  for(int i = 0; i < cmd.size(); i++){
    result[i] = strdup(cmd[i].c_str());
  }
  result[cmd.size()] = NULL;
  /*
  cout << "content in vecstrtochar:" << endl;
  for(int i = 0; i < cmd.size(); i++){
    string tmp(result[i]);
    cout << tmp << endl;
  }
  if (result[cmd.size()]==NULL){
    cout << "NULL" << endl;
  }
  */
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

void printStrVec(vector<string> v){
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

