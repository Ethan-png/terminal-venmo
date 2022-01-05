#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "DepositService.h"
#include "Database.h"
#include "ClientError.h"
#include "HTTPClientResponse.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

DepositService::DepositService() : HttpService("/deposits") { }

void DepositService::post(HTTPRequest *request, HTTPResponse *response) {
    if(request->hasAuthToken()){
        if(m_db->auth_tokens.count(request->getAuthToken())){
            auto feb = request->formEncodedBody();
            string amount = feb.get("amount");
            string stripe_token = feb.get("stripe_token");
            string currency = "USD";
            int amm = stoi(amount);
            if(amount.empty() || stripe_token.empty() || amm < 50){
                response->setStatus(401);
            }else{
                // connect to stripe api to generate charge id
                HttpClient client("api.stripe.com", 443, true);
                client.set_basic_auth(m_db->stripe_secret_key, "");
                WwwFormEncodedDict body;
                body.set("amount", amount);
                body.set("currency", currency);
                body.set("source", stripe_token);
                string encoded_body = body.encode();
                HTTPClientResponse *client_response = client.post("/v1/charges", encoded_body);
                if(!client_response->success()){
                    response->setStatus(401);
                }else{
                    Document *d = client_response->jsonBody();
                    string value = (*d)["id"].GetString();
                    delete d;
                    //update user bal & create deposit object
                    getAuthenticatedUser(request)->balance+=amm;
                    Deposit* newDeposit = new Deposit;
                    newDeposit->amount = amm;
                    newDeposit->stripe_charge_id = value;
                    newDeposit->to = getAuthenticatedUser(request);
                    //insert deposit object into database
                    m_db->deposits.push_back(newDeposit);
                    //DOCUMENT
                    Document document;
                    Document::AllocatorType& a = document.GetAllocator();
                    Value o;
                    o.SetObject();
                    o.AddMember("balance", getAuthenticatedUser(request)->balance, a);
                    Value array;
                    array.SetArray();
                    for(int i = 0; i < int(m_db->deposits.size()); i++){
                        if(m_db->deposits[i]->to->username == getAuthenticatedUser(request)->username){
                            Value to;
                            to.SetObject();
                            to.AddMember("to", m_db->deposits[i]->to->username, a);
                            to.AddMember("amount", m_db->deposits[i]->amount, a);
                            to.AddMember("stripe_charge_id", m_db->deposits[i]->stripe_charge_id, a);
                            array.PushBack(to, a);
                        }
                    }
                    o.AddMember("deposits", array, a);
                    document.Swap(o);
                    StringBuffer buffer;
                    PrettyWriter<StringBuffer> writer(buffer);
                    document.Accept(writer);

                    // set the return object
                    response->setContentType("application/json");
                    response->setBody(buffer.GetString() + string("\n"));
                    response->setStatus(200);
                }
            }
        }else{
            response->setStatus(401);
            throw ClientError::unauthorized();
        }
    }else{
        response->setStatus(401);
        throw ClientError::unauthorized();
    }
}
