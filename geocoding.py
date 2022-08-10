import json
import requests

def request_handler(request):

    if request["method"] == "GET":

        location = request["values"]["location"]

        getRequest = """https://api.mapbox.com/geocoding/v5/mapbox.places/{}.json?""".format(location)
        getRequest += """limit=1&proximity=-71.093086,42.358181&types=place,poi,address"""
        getRequest += """&access_token=pk.eyJ1Ijoic2FtYXlnb2Rpa2EiLCJhIjoiY2wyYzByczlnMGFjMDNjbXhsa3EycmpscyJ9.5ak4IQW4G5wluwPxIDZYGA"""
                        
        # return getRequest
        r = requests.get(getRequest)
        response = json.loads(r.text)

        return {"longitude": response['features'][0]['center'][0], "latitude": response['features'][0]['center'][1]}
        return response

    else:
        return "error"


# dic = {'type': 'FeatureCollection', 'query': ['prudential', 'center'], 'features': [{'id': 'poi.609885376421', 'type':
# 'Feature', 'place_type': ['poi'], 'relevance': 1, 'properties': {'foursquare': '4a54a236f964a5202db31fe3', 'landmark':
# True, 'address': '800 Boylston St', 'category': 'business, service, building'}, 'text': 'Prudential Center Tower',
# 'place_name': 'Prudential Center Tower, 800 Boylston St, Boston, Massachusetts 02199, United States', 'center':
# [-71.082487, 42.347087], 'geometry': {'coordinates': [-71.082487, 42.347087], 'type': 'Point'}, 'context': [{'id':
# 'neighborhood.2400071126510821', 'text': 'Prudential'}, {'id': 'postcode.7546133011393040', 'text': '02199'}, {'id':
# 'place.5300947049012190', 'wikidata': 'Q100', 'text': 'Boston'}, {'id': 'district.11510878044935260', 'wikidata':
# 'Q54072', 'text': 'Suffolk County'}, {'id': 'region.8307399429561540', 'short_code': 'US-MA', 'wikidata': 'Q771',
# 'text': 'Massachusetts'}, {'id': 'country.14135384517372290', 'wikidata': 'Q30', 'short_code': 'us', 'text': 'United States'}]}]