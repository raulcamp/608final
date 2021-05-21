import os
import requests
import json
from bs4 import BeautifulSoup
import random
# import spotipy
# from spotipy.oauth2 import SpotifyOAuth



offset = 80
nums = [80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 203]


os.environ['SPOTIPY_CLIENT_ID'] = '77998472009b47bd8a9df97eb64db805'
os.environ['SPOTIPY_CLIENT_SECRET'] = 'a3bd6d99492c457c944c29d64b690ab5'
os.environ['SPOTIPY_REDIRECT_URI'] = 'http://mysite.com/callback/'

TRACK_IDS = {
    'Peaches': '4iJyoBOLtHqaGxP12qzhQI',
    'ATW': '1pKYYY0dkg23sQQXi0Q5zN',
    'Animals': '1xMLthTaWJieT9YGV1hyS5',
    'OneMoreTime': '0DiWol3AO6WpXZgp0goxAV',
    'Toccata': '5cYyCE4b3T5TdQ3sGg8CYc',
    'LoseYourselfToDance': '5CMjjywI0eZMixPeqNd75R',
    'ForABetterDay': '7kbTZWt7DnzIzbkyzFE1PW'
}
'''
class SpotifyManager:
    def __init__(self) -> None:
        scope = 'user-modify-playback-state user-top-read user-read-currently-playing'
        auth = SpotifyOAuth(scope=scope)
        self.sp = spotipy.Spotify(auth_manager=auth)

    def get_current_song(self):
        result = self.sp.currently_playing()
        return {'id': result['item']['id'], 'progress': result['item']['progresss_ms']}

    def get_beats(self):
        id = self.get_current_song()['id']
        audio_analysis = self.sp.audio_analysis(id)
        beats_data = audio_analysis['beats']
        start_beats = [beat['start'] for beat in beats_data]
        return self.get_str(start_beats)

    def get_segments(self):
        id = self.get_current_song()['id']
        audio_analysis = self.sp.audio_analysis(id)
        segments = audio_analysis['segments']
        start_segments = [seg['start'] for seg in segments]
        return self.get_str(start_segments)

    def get_str(arr):
        l = len(arr)
        s = f'{l},'
        for e in arr:
            s += f'{e},'
        return s
'''

SPOTIFY_POST_TOKEN = 'BQBs4lvks7rrCGBmdHNH9kbcMhwRQs1Ac4OXQaUogvwLR_WYn_yYCCrfic99qJuquPWuc66Qlulsbtquecq_opbWlqFEuj-qOcGXD1f9q1-7uQINEJuqQ_jZJeYWv_8TSIC2X8asRjPxsMIMNANDNQ'
SPOTIFY_ACCESS_TOKEN = 'BQDtnnM0EyPdFWFwyPRZON97yRc3bJJu_8E7Ky6XjizvCkPcwmXaKVlAEDNeFQaZ5JCnyRqJAbta6frwoPFnsWRfrwsM3-GDwoY4o0Ncx-nH4atR2uGC41MSYBeCTMP1BF-RgJF2e4rjEge0jTjZmQ'
SPOTIFY_GET_CURRENT_TRACK_URL = 'https://api.spotify.com/v1/me/player/currently-playing'
SPOTIFY_GET_AUDIO_ANALYSIS_URL = 'https://api.spotify.com/v1/audio-analysis/'

SPOTIFY_GET_AUDIO_FEATURES_URL = 'https://api.spotify.com/v1/audio-features/'

SPOTIFY_GET_SONG_ID = "https://api.spotify.com/v1/search"



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

    return title, artist

def get_segments():
    cur_song = requests.get(
        SPOTIFY_GET_CURRENT_TRACK_URL,
        headers={
            "Authorization": f"Bearer {SPOTIFY_ACCESS_TOKEN}"
        }
    )
    id = cur_song.json()['item']['id']
    response = requests.get(
        SPOTIFY_GET_AUDIO_ANALYSIS_URL + id,
        headers={
            "Authorization": f"Bearer {SPOTIFY_ACCESS_TOKEN}"
        }
    )

    resp_json = response.json()

    segments_data = resp_json['segments']

    return [seg['start'] for seg in segments_data]


def get_str(arr):
    l = len(arr)
    s = f'{l},'
    for e in arr:
        s += f'{e},'
    return s



def get_beats():
    cur_song = requests.get(
        SPOTIFY_GET_CURRENT_TRACK_URL,
        headers={
            "Authorization": f"Bearer {SPOTIFY_ACCESS_TOKEN}"
        }
    )
    id = cur_song.json()['item']['id']
    response = requests.get(
        SPOTIFY_GET_AUDIO_ANALYSIS_URL + id,
        headers={
            "Authorization": f"Bearer {SPOTIFY_ACCESS_TOKEN}"
        }
    )

    resp_json = response.json()

    beats_data = resp_json['beats']

    return [beat['start'] for beat in beats_data]

def get_progress(): #returns artist, title
    response = requests.get(
        SPOTIFY_GET_CURRENT_TRACK_URL,
        headers={
            "Authorization": f"Bearer {SPOTIFY_ACCESS_TOKEN}"
        }
    )
    data = response.json()
    # return data
    return data['item']['album']['artists'][0]['name'], data['item']['name']

    # return data['item']['name']

def get_danceability():
    cur_song = requests.get(
        SPOTIFY_GET_CURRENT_TRACK_URL,
        headers={
            "Authorization": f"Bearer {SPOTIFY_ACCESS_TOKEN}"
        }
    )
    id = cur_song.json()['item']['id']
    response = requests.get(
        SPOTIFY_GET_AUDIO_FEATURES_URL + id,
        headers={
            "Authorization": f"Bearer {SPOTIFY_ACCESS_TOKEN}"
        }
    )

    resp_json = response.json()

    danceability = resp_json['danceability']

    return danceability

# "https://api.spotify.com/v1/search?q=track:"' + despacito + '"%20artist:"' + bieber + '"&type=track"

def convert_song_to_id(artist, song):

    artist = artist.replace(" ", "%20")
    song = song.replace(" ", "%20")

    # song = "despacito"
    response = requests.get(
                headers={
            "Authorization": f"Bearer {SPOTIFY_ACCESS_TOKEN}"
        },

        url= "https://api.spotify.com/v1/search?q=track:" + song + "%20artist:" + artist + "&type=track"
    )
    data = response.json()
    return data['tracks']['items'][0]['uri']


def add_to_queue(track_uri):
    # track_uri = "spotify:track:2D2zAfEkgY4G49idbWiezo"
    response = requests.post(
            headers={
            "Authorization": f"Bearer {SPOTIFY_POST_TOKEN}"
            },
            url = "https://api.spotify.com/v1/me/player/queue?uri=" + track_uri
        
    )
    return response.text

    # data = response.json()
    # return data

def first_time_interval(t=60000):#30 seconds = 30,000 milliseconds
    response = requests.get(
        SPOTIFY_GET_CURRENT_TRACK_URL,
        headers={
            "Authorization": f"Bearer {SPOTIFY_ACCESS_TOKEN}"
        }
    )
    data = response.json()
    # return data
    current_time = data['progress_ms']
    song_duration = data['item']['duration_ms']

    # return data
    return ((song_duration - current_time) < t)
    # return current_time, song_duration

def last_time_interval(t=20000):#30 seconds = 30,000 milliseconds
    response = requests.get(
        SPOTIFY_GET_CURRENT_TRACK_URL,
        headers={
            "Authorization": f"Bearer {SPOTIFY_ACCESS_TOKEN}"
        }
    )
    data = response.json()
    current_time = data['progress_ms']
    song_duration = data['item']['duration_ms']

    # return data
    return ((song_duration - current_time) < t)
    # return current_time, song_duration


'''

1) Do nothing until < 30 seconds left in the song
2) Notify esp32 to start counting steps... do this by returning "Start"
3) Notify esp32 to end counting steps...do this by returning "End"
4) If request args contain steps, then parse the request
    -Call steps to bpm converter
    -Retrieve that song reccomendation
    -Post that request to spotify and add the song to the queue

'''

def request_handler(request):
    if request['method'] == 'GET':
        if 'steps' in request['values']:
            if 'Start' in request['values']['steps']:
                # return first_time_interval(), last_time_interval()
                if first_time_interval() == True and last_time_interval() == False:
                    return 'Start'
                elif first_time_interval() == True and last_time_interval() == True:
                    return 'End'
                else:
                    return get_progress()
            else:
                step_count = int(request['values']['steps'])
                rec_track, rec_artist = recommend_song_by_bpm(step_count+offset)
                track_id = convert_song_to_id(rec_artist, rec_track)
                add_to_queue(track_id)
                return "New Song Added to Queue"

