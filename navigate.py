from urllib import response
import requests

def request_handler(request):
    dest_lon  = float(request['form']['destlon'])
    dest_lat = float(request['form']['destlat'])

    outs = "Directions\n"

    try:
        if request["method"] == "POST":
            try:
                startlon = float(request['form']['startlon'])
                startlat = float(request['form']['startlat'])

                txt = 'https://api.mapbox.com/directions/v5/mapbox/driving/{}%2C{}%3B{}%2C{}?alternatives=true&geometries=geojson&language=en&overview=simplified&steps=true&access_token=pk.eyJ1Ijoic2FtYXlnb2Rpa2EiLCJhIjoiY2wyYzBwNHl5MDF6ZzNqbzNmM3p3ZGJxNCJ9.NtOXKbVg7nxc9DFMYU5amw'.format(startlon, startlat, dest_lon, dest_lat)
                
                r = requests.get(txt)
                response = r.json()

                #now, need to construct the string response z 

                for route in response["routes"]:
                    for leg in route["legs"]:
                        for step in leg["steps"]:
                            outs += step["maneuver"]["instruction"]
                            outs += '\n'

                return outs

                for route in response["routes"]:
                    inner = route["legs"]
                    outs += inner["maneuver"]["instruction"]
                    outs += '\n'

                
                return outs

            except Exception as e:
                return outs
    except Exception as e:
        return "Error: make sure correct arguments in request are given."
