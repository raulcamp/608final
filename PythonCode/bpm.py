import requests
import json
from bs4 import BeautifulSoup
import random

# OATH_TOKEN = 'BQDK0MNbSWKgYY5ujw1H6RlU13FVeQlPORdiwoIPuUsQuYxTZSKRKHHTXyTJma4srHwDoPMyloTZ6h7kLXJA-MUKc_DcqIs9VkIXBi5uYb7RXT5HE_DAVvG4H0S_98ykwsu_RtSokHrQ1BkPhJPzMov8-eQg31rBEZYrNHd0RKQ'

# OAUTH_TOKEN = f'Bearer {OATH_TOKEN}'

nums = [80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 203]

def get_current_song(token, market='US', additional_types='episode'):
    endpoint = 'https://api.spotify.com/v1/me/player/currently-playing'
    OAUTH_TOKEN = f'Bearer {token}'
    res = requests.get(endpoint, params={'market': market, 'additional_types': additional_types}, headers={'Authorization': OAUTH_TOKEN})

    content = json.loads(res.content.decode('utf-8'))

    song = content['item']
    
    artist = song['artists'][0]['name']
    song_title = song['name']

    return song_title + " by " + artist

def get_closest_value(value, sorted_list):
    if value < min(sorted_list): return min(sorted_list)
    elif value > max(sorted_list): return max(sorted_list)

    for i,j in zip(sorted_list[:-1], sorted_list[1:]):
        if value == i: return i
        if value == j: return j

        if i < value < j:
            from_i = abs(value - i)
            from_j = abs(j - value)

            if from_i == from_j: return i
            elif from_i > from_j: return j
            else: return i

    return -1

def recommend_song_by_bpm(bpm):
    nearest_bpm = get_closest_value(bpm, nums)

    url = f'https://www.cs.ubc.ca/~davet/music/bpm/{nearest_bpm}.html'
    res = requests.get(url)
    
    # parse html
    soup = BeautifulSoup(res.content, 'html.parser')

    soup = soup.find('table', {'class': 'music'})
    songs = soup.findAll('tr', {'class': None})

    song = random.choice(songs)

    _,artist,title,_,_,_,_,_,_ = song.findAll('td')

    artist = artist.text
    title = title.text

    return f'{title} by {artist}'

def request_handler(request):
    if request['method'] == 'GET':
        if 'bpm' in request['values']:
            return recommend_song_by_bpm(int(request['values']['bpm']))
        else:
            return get_current_song(request['values']['token'])

    return 'INVALID'
