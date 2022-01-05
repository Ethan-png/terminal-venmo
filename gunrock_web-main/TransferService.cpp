#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "TransferService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

TransferService::TransferService() : HttpService("/transfers") { }


void TransferService::post(HTTPRequest *request, HTTPResponse *response) {
    if(request->hasAuthToken()){
        if(m_db->auth_tokens.count(request->getAuthToken())){
            auto feb = request->formEncodedBody();
            string to = feb.get("to");
            string amount = feb.get("amount");
            int amm = stoi(amount);
            //check if missing any arguments.
            if(to == "" || amount == ""){
                response->setStatus(400);
            }
            //check if balance is negative
            else if(amm < 0){
                response->setStatus(400);
            }
            //if amm being transfered is more than current bal
            else if(amm > getAuthenticatedUser(request)->balance){
                response->setStatus(400);
            }
            else{
                //check if to exist
                if(m_db->users.count(to)){
                    User* transferUser = new User;
                    //initate transfer
                    transferUser = m_db->users[to];
                    if(transferUser->user_id == getAuthenticatedUser(request)->user_id){
                        response->setStatus(400);
                    }else{
                    transferUser->balance+=amm;
                    getAuthenticatedUser(request)->balance-=amm;
                    //create transfer log
                    Transfer* newTransfer = new Transfer;
                    newTransfer->amount = amm;
                    newTransfer->from = getAuthenticatedUser(request);
                    newTransfer->to = transferUser;
                    m_db->transfers.push_back(newTransfer);
                    //DOCUMENT
                    Document document;
                    Document::AllocatorType& a = document.GetAllocator();
                    Value o;
                    o.SetObject();
                    o.AddMember("balance", getAuthenticatedUser(request)->balance, a);
                    Value array;
                    array.SetArray();
                    for(int i = 0; i < m_db->transfers.size(); i++){
                        if(m_db->transfers[i]->from->username == getAuthenticatedUser(request)->username){
                            Value to;
                            to.SetObject();
                            to.AddMember("from", m_db->transfers[i]->from->username, a);
                            to.AddMember("to", m_db->transfers[i]->to->username, a);
                            to.AddMember("amount", m_db->transfers[i]->amount, a);
                            array.PushBack(to, a);
                        }
                        else if(m_db->transfers[i]->to->username == getAuthenticatedUser(request)->username){
                            Value to;
                            to.SetObject();
                            to.AddMember("from", m_db->transfers[i]->from->username, a);
                            to.AddMember("to", m_db->transfers[i]->to->username, a);
                            to.AddMember("amount", m_db->transfers[i]->amount, a);
                            array.PushBack(to, a);
                        }
                    }
                    o.AddMember("transfers", array, a);
                    document.Swap(o);
                    StringBuffer buffer;
                    PrettyWriter<StringBuffer> writer(buffer);
                    document.Accept(writer);

                    // set the return object
                    response->setContentType("application/json");
                    response->setBody(buffer.GetString() + string("\n"));
                    response->setStatus(200);
                    }
                }else{
                    response->setStatus(400);
                }
            }
        }else{
            throw ClientError::unauthorized();
        }
    }else{
        throw ClientError::unauthorized();
    }
}
