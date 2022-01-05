#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AccountService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AccountService::AccountService() : HttpService("/users") {
  
}

void AccountService::get(HTTPRequest *request, HTTPResponse *response) {
    if(request->hasAuthToken()){
        string user_id = request->getPathComponents()[1];
        string auth_token = request->getAuthToken();
        if(user_id.empty() || auth_token.empty()){
            response->setStatus(401);
        }
        else if(getAuthenticatedUser(request) != NULL){
            if(getAuthenticatedUser(request)->user_id == user_id){
                Document document;
                Document::AllocatorType& a = document.GetAllocator();
                Value o;
                o.SetObject();
                o.AddMember("balance", getAuthenticatedUser(request)->balance, a);
                o.AddMember("email", getAuthenticatedUser(request)->email, a);
                document.Swap(o);
                StringBuffer buffer;
                PrettyWriter<StringBuffer> writer(buffer);
                document.Accept(writer);
                response->setContentType("application/json");
                response->setBody(buffer.GetString() + string("\n"));
                response->setStatus(200);
            }else{
                response->setStatus(403);
            }
        }else{
            response->setStatus(401);
        }
    }else{
        response->setStatus(401);
        throw ClientError::unauthorized();
    }
}

void AccountService::put(HTTPRequest *request, HTTPResponse *response) {
    auto feb = request->formEncodedBody();
    string email = feb.get("email");
    if(!email.empty()){
        if(request->hasAuthToken()){
            string user_id = request->getPathComponents()[1];
            if(getAuthenticatedUser(request)->user_id == user_id){
                getAuthenticatedUser(request)->email = email;
                Document document;
                Document::AllocatorType& a = document.GetAllocator();
                Value o;
                o.SetObject();
                o.AddMember("balance", getAuthenticatedUser(request)->balance, a);
                o.AddMember("email", getAuthenticatedUser(request)->email, a);
                document.Swap(o);
                StringBuffer buffer;
                PrettyWriter<StringBuffer> writer(buffer);
                document.Accept(writer);
                response->setContentType("application/json");
                response->setBody(buffer.GetString() + string("\n"));
                response->setStatus(200);
            }else{
                //userid & auth token does not match
                response->setStatus(403);
            }
        }else{
            response->setStatus(401);
            throw ClientError::unauthorized();
        }
    }else{
        response->setStatus(400);
    }
}
