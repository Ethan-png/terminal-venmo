#define RAPIDJSON_HAS_STDSTRING 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "WwwFormEncodedDict.h"
#include "HttpClient.h"
#include "StringUtils.h"
#include "rapidjson/document.h"
#include "vector"

using namespace std;
using namespace rapidjson;

int API_SERVER_PORT = 8080;
string API_SERVER_HOST = "localhost";
string PUBLISHABLE_KEY = "";

string auth_token;
string user_id;
string username;
//gets input
string getInput(){
    string input;
    cout << "D$> ";
    getline(cin, input);
    return input;
}

void Error(){
  cout << "Error" << endl;
}

bool validCommand(string argument){
  if(argument == "auth" || argument == "balance" || argument == "deposit" || argument == "send" || argument == "logout"){
    return true;
  }
  return false;
}

void auth(vector<string> arguments){
    //initiate session
  HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
  WwwFormEncodedDict body;
  username = arguments[1];
  body.set("username", arguments[1]);
  body.set("password", arguments[2]);
  string encoded_body = body.encode();
  HTTPClientResponse *client_response = client.post("/auth-tokens", encoded_body);
  if(!client_response->success()){
    Error();
  }else{ 
    //read response and retrieve auth_token and user_id
    Document *d = client_response->jsonBody();
    auth_token = (*d)["auth_token"].GetString();
    user_id = (*d)["user_id"].GetString();
    HttpClient getbal(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
    getbal.set_header("x-auth-token", auth_token);
    string path = "/users/"+user_id;
    HTTPClientResponse *balan = getbal.get(path);
    Document *f = balan->jsonBody();
    double balance = double((*f)["balance"].GetInt());
    cout << "Balance: $";
    printf("%.2lf",balance/100);
    cout << endl;
  }
}

void emailauth(vector<string> arguments){
    //initiate session
  HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
  WwwFormEncodedDict body;
  username = arguments[1];
  body.set("username", arguments[1]);
  body.set("password", arguments[2]);
  string encoded_body = body.encode();
  HTTPClientResponse *client_response = client.post("/auth-tokens", encoded_body);
  if(!client_response->success()){
    Error();
  }else{
    //read response and retrieve auth_token and user_id
    Document *d = client_response->jsonBody();
    auth_token = (*d)["auth_token"].GetString();
    user_id = (*d)["user_id"].GetString();
    //get user info
    HttpClient getbal(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
    //set header and path
    getbal.set_header("x-auth-token", auth_token);
    string path = "/users/"+user_id;
    //add email arg
    WwwFormEncodedDict a;
    a.set("email", arguments[3]);
    string encoded_a = a.encode();
    //call put command
    HTTPClientResponse *balan = getbal.put(path, encoded_a);
    Document *f = balan->jsonBody();
    double balance = double((*f)["balance"].GetInt());
    cout << "Balance: $";
    printf("%.2lf",balance/100);
    cout << endl;
  }
}

void balance(){
  HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
  string path = "/users/"+user_id;
  client.set_header("x-auth-token", auth_token);
  HTTPClientResponse *client_response = client.get(path);
  Document *d = client_response->jsonBody();
  double balance = double((*d)["balance"].GetInt());
  cout << "Balance: $";
  printf("%.2lf",balance/100);
  cout << endl;
}

void deposit(vector<string> arguments){
  HttpClient stripe("api.stripe.com", 443, true);
  stripe.set_basic_auth(PUBLISHABLE_KEY, "");
  WwwFormEncodedDict stripebody;
  stripebody.set("card[number]", arguments[2]);
  stripebody.set("card[exp_month]", arguments[4]);
  stripebody.set("card[exp_year]", arguments[3]);
  stripebody.set("card[cvc]", arguments[5]);
  string encoded_stripebody = stripebody.encode();
  HTTPClientResponse *stripe_response = stripe.post("/v1/tokens", encoded_stripebody);
  Document *d = stripe_response->jsonBody();
  string token = (*d)["id"].GetString();
  delete d;
  HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
  client.set_header("x-auth-token", auth_token);
  WwwFormEncodedDict body;
  int a = stoi(arguments[1]);
  string b = to_string(a);
  body.set("amount", b);
  body.set("stripe_token", token);
  string encoded_body = body.encode();
  client.post("/deposits", encoded_body);
  balance();
}

void send(vector<string> arguments){
  HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
  client.set_header("x-auth-token", auth_token);
  WwwFormEncodedDict body;
  body.set("to", arguments[1]);
  body.set("amount", to_string(stoi(arguments[2])*100));
  string encoded_body = body.encode();
  client.post("/transfers", encoded_body);
  balance();
}

void logout(){
  HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
  string path = "/auth-tokens/"+auth_token;
  client.set_header("x-auth-token", auth_token);
  client.del(path);
  exit(0);
}
int main(int argc, char *argv[]) {
  stringstream config;
  int fd = open("config.json", O_RDONLY);
  if (fd < 0) {
    cout << "could not open config.json" << endl;
    exit(1);
  }
  int ret;
  char buffer[4096];
  while ((ret = read(fd, buffer, sizeof(buffer))) > 0) {
    config << string(buffer, ret);
  }
  Document d;
  d.Parse(config.str());
  API_SERVER_PORT = d["api_server_port"].GetInt();
  API_SERVER_HOST = d["api_server_host"].GetString();
  PUBLISHABLE_KEY = d["stripe_publishable_key"].GetString();
  if(argc == 2){
    string line;
    ifstream myfile(argv[1]);
    if(myfile.is_open()){
      while(getline(myfile,line)){
        string input = line;
        vector<string> arguments = StringUtils::split(input, ' ');
        if(validCommand(arguments[0])){
          //Auth
          if(arguments[0] == "auth"){
            if(arguments.size() == 3)
            {
              auth(arguments);
            }
            else if(arguments.size() == 4)
            {
              emailauth(arguments);
            }
            else{
              Error();
            }
          }
          else if(arguments[0] == "balance" && !auth_token.empty() && !user_id.empty()){
            if(arguments.size() == 1){
              balance();
            }else{
              Error();
            }
          }
          else if(arguments[0] == "send" && !auth_token.empty() && !user_id.empty()){
            if(arguments.size() == 3){
              send(arguments);
            }else{
              Error();
            }
          }
          else if(arguments[0] == "deposit" && !auth_token.empty() && !user_id.empty()){
            if(arguments.size() == 6){
              deposit(arguments);
            }else{
              Error();
            }
          }
          else if(arguments[0] == "logout"){
            logout();
          }
        }else{
          Error();
        }
      }
    }
  }
  else{
    while(true){
      string input = getInput();
      vector<string> arguments = StringUtils::split(input, ' ');
      if(validCommand(arguments[0])){
        //Auth
        if(arguments[0] == "auth"){
          if(arguments.size() == 3)
          {
            auth(arguments);
          }
          else if(arguments.size() == 4)
          {
            emailauth(arguments);
          }
          else{
            Error();
          }
        }
        else if(arguments[0] == "balance" && !auth_token.empty() && !user_id.empty()){
          if(arguments.size() == 1){
            balance();
          }else{
            Error();
          }
        }
        else if(arguments[0] == "send" && !auth_token.empty() && !user_id.empty()){
          if(arguments.size() == 3){
            send(arguments);
          }else{
            Error();
          }
        }
        else if(arguments[0] == "deposit" && !auth_token.empty() && !user_id.empty()){
          if(arguments.size() == 6){
            if(arguments[2].length() != 16){
              Error();
            }else{
              deposit(arguments);
            }
          }else{
            Error();
          }
        }
        else if(arguments[0] == "logout"){
          logout();
        }
      }else{
        Error();
      }
    }
  }
  return 0;
}
