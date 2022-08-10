import json
import requests

def request_handler(request):

    if request["method"] == "GET":

        location = request["values"]["location"]

        getRequest = """https://api.mapbox.com/geocoding/v5/mapbox.places/{}.json?""".format(location)
        getRequest += """limit=1&proximity=-71.093086,42.358181&types=place,poi,address"""
        getRequest += #removed access token for security
                        
        # return getRequest
        r = requests.get(getRequest)
        response = json.loads(r.text)

        return {"longitude": response['features'][0]['center'][0], "latitude": response['features'][0]['center'][1]}
        return response

    else:
        return "error"
