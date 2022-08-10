from twilio.rest import Client

ACCOUNT_ID = #removed for security
AUTH_TOKEN = #removed for security
FROM_PHONE = #insert from phone num
TO_PHONE = #insert to phone num
def request_handler(request) :

    if(request["method"] == 'POST'):
        try:
            message = request['form']['message']

            # Find your Account SID and Auth Token at twilio.com/console
            # and set the environment variables. See http://twil.io/secure
            account_sid = ACCOUNT_ID
            auth_token = AUTH_TOKEN
            client = Client(account_sid, auth_token)

            client.messages.create(
                    body=message,
                    from_=FROM_PHONE,
                    to=TO_PHONE
                )
            return "success"

        except:
            return "error"
    
    else:
        return "error"

        
