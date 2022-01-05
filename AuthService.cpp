#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AuthService.h"
#include "StringUtils.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AuthService::AuthService() : HttpService("/auth-tokens") {
  
}

bool lowercase(string a){
  for(int i = 0; i<int(a.length()); i++){
    if(!islower(a[i])){
      return false;
    }
  }
  return true;
}

void AuthService::post(HTTPRequest *request, HTTPResponse *response) {
      auto feb = request->formEncodedBody();
      string username = feb.get("username");
      string password = feb.get("password");
      if(username.empty() || password.empty() || !lowercase(username)){
        response->setStatus(401);
      }else{
      string auth_token = StringUtils::createAuthToken();
      //check if username is in database
      if(m_db->users.count(username)){
        User* login = new User;
        login = m_db->users[username];
        if(login->password == password){
          //login successful, generate auth token. 
          m_db->auth_tokens.insert(make_pair(auth_token, login));
          Document document;
          Document::AllocatorType& a = document.GetAllocator();
          Value o;
          o.SetObject();
          o.AddMember("auth_token", auth_token, a);
          o.AddMember("user_id", login->user_id, a);
          document.Swap(o);
          StringBuffer buffer;
          PrettyWriter<StringBuffer> writer(buffer);
          document.Accept(writer);
          response->setContentType("application/json");
          response->setBody(buffer.GetString() + string("\n"));
          response->setStatus(200);
        }else{
          //login unsuccessful wrong password
          response->setStatus(403);
        }
      }else{
        //username not in database must create. 
        User* newUser = new User;
        newUser->user_id = StringUtils::createUserId();
        newUser->username = username;
        newUser->password = password;
        newUser->balance = 0;
        newUser->email = "";
        m_db->users.insert(make_pair(username, newUser));
        m_db->auth_tokens.insert(make_pair(auth_token, newUser));
        Document document;
        Document::AllocatorType& a = document.GetAllocator();
        Value o;
        o.SetObject();
        o.AddMember("auth_token", auth_token, a);
        o.AddMember("user_id", newUser->user_id, a);
        document.Swap(o);
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        document.Accept(writer);
        response->setContentType("application/json");
        response->setBody(buffer.GetString() + string("\n"));
        response->setStatus(201);
      }
      }
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response) {
    vector<string> params = request->getPathComponents();
    if(request->hasAuthToken()){
        string auth_token = request->getAuthToken();
        if(m_db->auth_tokens.count(auth_token)){
          std::map<std::string, User *>::iterator it;
          it = m_db->auth_tokens.find(params[params.size()-1]);
          m_db->auth_tokens.erase(it);
          response->setStatus(200);
        }else{
        response->setStatus(404);
        }
    }else{
        response->setStatus(404);
        throw ClientError::unauthorized();
    }
}