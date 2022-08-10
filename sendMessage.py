from twilio.rest import Client

ACCOUNT_ID = 'ACbbcf75776af41239478e20ea01030344'
AUTH_TOKEN = 'd5c9888ad83c45da1437d40b54335ebe'
FROM_PHONE = "+19707166492"
TO_PHONE = "+16177753891"
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

        
