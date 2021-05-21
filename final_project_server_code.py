import sqlite3
import datetime
import random
import string
import json
import pprint
import requests
# import spotipy
# from spotipy.oauth2 import SpotifyClientCredentials
group_db = '/var/jail/home/team65/raul/group.db'
invited_db = '/var/jail/home/team65/raul/invited.db'
liked_db = '/var/jail/home/team65/raul/liked.db'
shared_db = '/var/jail/home/team65/raul/shared.db'

def request_handler(request):
    if request['method'] == "GET":
        action = request['values']['action']
        if action == "get_song":
            token = request['values']['token']
            endpoint = 'https://api.spotify.com/v1/me/player/currently-playing'
            market = 'US'
            additional_types='episode'
            OAUTH_TOKEN = f'Bearer {token}'
            res = requests.get(endpoint, params={'market': market, 'additional_types': additional_types}, headers={'Authorization': OAUTH_TOKEN})

            content = json.loads(res.content.decode('utf-8'))
            song = content['item']

            artist = song['artists'][0]['name']
            song_title = song['name']
            uri = song['uri']

            return song_title + " by " + artist + "`" + uri
        username = request['values']['username']
        if action == "get_invites":
            with sqlite3.connect(invited_db) as cc:
                cc.execute('''CREATE TABLE IF NOT EXISTS invited_data (group_name text, username text);''')
                invited_groups = cc.execute('''SELECT group_name FROM invited_data WHERE username = ?;''',(username,)).fetchall()
                invited_groups = [group for (group,) in invited_groups]
                if len(invited_groups) > 0:
                    return invited_groups[0]
                else:
                    return "NO INVITES"
                # if len(invited_groups) == 0:
                #     return "You have not been invited to join any groups"
                # elif len(invited_groups) == 1:
                #     return "You have been invited to join the group {}.".format(invited_groups[0])
                # elif len(invited_groups) == 2:
                #     return "You have been invited to join the groups {}.".format(' and '.join(invited_groups))
                # else:
                #     group_string = ', '.join(invited_groups[:-1])
                #     group_string += ', and '
                #     group_string += invited_groups[-1]
                #     return "You have been invited to join the groups {}.".format(group_string)

        elif action == "get_groups":
            with sqlite3.connect(group_db) as c:
                c.execute('''CREATE TABLE IF NOT EXISTS group_data (group_name text, username text, passcode text);''')
                group_users = c.execute('''SELECT group_name FROM group_data WHERE username = ?;''',(username,)).fetchall()
                data = dict()
                data["groups"] = []
                for (group,) in group_users:
                    data["groups"].append(group)
                data["num_groups"] = len(group_users)
                return json.dumps(data)

        elif action == "get_likes":
            group_name = request['values']['group_name']
            with sqlite3.connect(liked_db) as ccc:
                with sqlite3.connect(group_db) as c:
                    current_users = c.execute('''SELECT username FROM group_data WHERE group_name = ?;''',(group_name,)).fetchall()
                    if len(current_users) == 0:
                        return "The group {} does not exist.".format(group_name)
                    if (username,) not in current_users:
                        return "You are not in the group {}.".format(group_name)
                    ccc.execute('''CREATE TABLE IF NOT EXISTS liked_data (group_name text, liker text, song text);''')
                    liked_songs = ccc.execute('''SELECT * FROM liked_data WHERE group_name = ?;''',(group_name,)).fetchall()
                    liked_songs = [{'song': song, 'liker': liker} for (group_name, liker, song,) in liked_songs]
                    return liked_songs
                        
        elif action == "get_shared":
            with sqlite3.connect(shared_db) as cccc:
                cccc.execute('''CREATE TABLE IF NOT EXISTS shared_data (group_name text, username text, sharer text, song text);''')
                shared_songs = cccc.execute('''SELECT * FROM shared_data WHERE username = ?;''',(username,)).fetchall()
                cccc.execute('''DELETE from shared_data WHERE username = ?;''',(username,))
                shared_songs = [{'song': song, 'sharer': sharer, 'group': group_name} for (group_name, username, sharer, song,) in shared_songs]
                if len(shared_songs) > 0:
                    return shared_songs[0]['song']
                else:
                    return "NO SHARED"
        else:
            return "The action {} does not exist.".format(action)


            
    if request['method'] == "POST":
        action = request['form']['action']
        if action == "play_song":
            song = request['form']['song']
            token = request['form']['token']
            endpoint = 	'https://api.spotify.com/v1/me/player/queue'
            OAUTH_TOKEN = f'Bearer {token}'
            requests.post(endpoint, params={'uri': song}, headers={'Authorization': OAUTH_TOKEN})
            endpoint = 'https://api.spotify.com/v1/me/player/next'
            requests.post(endpoint, headers={'Authorization': OAUTH_TOKEN})
            return
        group_name = request['form']['group_name']
        if action == "make_playlist":
            token = request['form']['token']
            endpoint = 'https://api.spotify.com/v1/users/jayliner66/playlists' // jayliner66 is James's spotiy username
            OAUTH_TOKEN = f'Bearer {token}'
            request_body = json.dumps({"name": group_name, "description": "The liked songs in "+group_name, "public": False})
            requests.post(endpoint, data=request_body, headers={'Authorization': OAUTH_TOKEN, 'Accept': "application/json", "Content-type": "application/json"})
            with sqlite3.connect(liked_db) as ccc:
                ccc.execute('''CREATE TABLE IF NOT EXISTS liked_data (group_name text, liker text, song text);''')
                group_songs = ccc.execute('''SELECT song FROM liked_data WHERE group_name = ?;''',(group_name,)).fetchall()
            real_group_songs = []
            group_songs = [song.split('`')[1] for (song,) in group_songs]
            endpoint = 'https://api.spotify.com/v1/me/playlists'
            res = requests.get(endpoint, headers={'Authorization': OAUTH_TOKEN})
            content = json.loads(res.content.decode('utf-8'))
            playlists = content['items']
            playlist_id = ""
            for playlist in playlists:
                if playlist['name'] == group_name:
                    playlist_id = playlist['id']
            endpoint = 'https://api.spotify.com/v1/playlists/'+playlist_id+'/tracks'
            for song in group_songs:
                requests.post(endpoint, params={'uris': [song]}, headers={'Authorization': OAUTH_TOKEN, 'Accept': "application/json", "Content-type": "application/json"})
            return
        username = request['form']['username']
        with sqlite3.connect(group_db) as c:
            c.execute('''CREATE TABLE IF NOT EXISTS group_data (group_name text, username text, passcode text);''')
            current_users = c.execute('''SELECT username FROM group_data WHERE group_name = ?;''',(group_name,)).fetchall()
        with sqlite3.connect(invited_db) as cc:
            cc.execute('''CREATE TABLE IF NOT EXISTS invited_data (group_name text, username text);''')
        with sqlite3.connect(liked_db) as ccc:
            ccc.execute('''CREATE TABLE IF NOT EXISTS liked_data (group_name text, liker text, song text);''')
        with sqlite3.connect(shared_db) as cccc:
            cccc.execute('''CREATE TABLE IF NOT EXISTS shared_data (group_name text, username text, sharer text, song text);''')
        if action == "make":
            passcode = ''.join(random.choice(string.digits) for i in range(6))

            with sqlite3.connect(group_db) as c:
                current_groups = c.execute('''SELECT group_name FROM group_data ;''').fetchall()
                if (group_name,) in current_groups:
                    return "The group {} already exists.".format(group_name)
                c.execute('''INSERT into group_data VALUES (?,?,?);''',(group_name, username, passcode,))
            return "The group {} has been successfully formed! The group passcode is {}.".format(group_name, passcode)
        elif action == "invite":
            friend = request['form']['friend']

            with sqlite3.connect(invited_db) as cc:
                invited_users = cc.execute('''SELECT username FROM invited_data WHERE group_name = ?;''',(group_name,)).fetchall()

            with sqlite3.connect(group_db) as c:
                if len(current_users) == 0:
                    return "The group {} does not exist.".format(group_name)
                if (username,) not in current_users:
                    return "You are not in the group {}.".format(group_name)
                if (friend,) in current_users:
                    return "{} is already in the group {}.".format(friend, group_name)
                if (friend,) in invited_users:
                    return "{} has already been invited to the group {}.".format(friend, group_name)
                else:
                    with sqlite3.connect(invited_db) as cc:
                        cc.execute('''INSERT into invited_data VALUES (?,?);''',(group_name, friend,))
                    return "You have successfully invited {} to join the group {}.".format(friend, group_name)
                
        elif action == "join":
            passcode = request['form']['passcode']
            with sqlite3.connect(group_db) as c:
                if len(current_users) == 0:
                    return "The group {} does not exist.".format(group_name)
                actual_passcode = c.execute('''SELECT passcode FROM group_data WHERE group_name = ?;''', (group_name,)).fetchone()[0]
                if passcode == actual_passcode:
                    if (username,) in current_users:
                        return "You are already in the group {}.".format(group_name)
                    c.execute('''INSERT into group_data VALUES (?,?,?);''',(group_name, username, passcode,))
                    with sqlite3.connect(invited_db) as cc:
                        try:
                            cc.execute('''DELETE from invited_data WHERE group_name = ? AND username = ?);''',(group_name, username,))
                        except:
                            pass
                    return "You have successfully joined the group {}.".format(group_name)
                else:
                    return "The password is incorrect."
        elif action == "rejected_join":
            with sqlite3.connect(group_db) as c:
                if len(current_users) == 0:
                    return "The group {} does not exist.".format(group_name)
                if (username,) in current_users:
                    return "You are already in the group {}.".format(group_name)
                else:
                    with sqlite3.connect(invited_db) as cc:
                        invited_groups = cc.execute('''SELECT group_name FROM invited_data WHERE username = ?;''',(username,)).fetchall()
                        if (group_name,) not in invited_groups:
                            return "You have not been invited to the group {}.".format(group_name)
                        cc.execute('''DELETE from invited_data WHERE group_name = ? AND username = ?;''',(group_name, username,))
                        return "You have successfully rejected the invite to join the group {}.".format(group_name)
        elif action == "invited_join":
            with sqlite3.connect(group_db) as c:
                if len(current_users) == 0:
                    return "The group {} does not exist.".format(group_name)
                if (username,) in current_users:
                    return "You are already in the group {}.".format(group_name)
                else:
                    with sqlite3.connect(invited_db) as cc:
                        invited_groups = cc.execute('''SELECT group_name FROM invited_data WHERE username = ?;''',(username,)).fetchall()
                        if (group_name,) not in invited_groups:
                            return "You have not been invited to the group {}.".format(group_name)
                        passcode = c.execute('''SELECT passcode FROM group_data WHERE group_name = ?;''', (group_name,)).fetchone()[0]
                        c.execute('''INSERT into group_data VALUES (?,?,?);''',(group_name, username, passcode,))
                        cc.execute('''DELETE from invited_data WHERE group_name = ? AND username = ?;''',(group_name, username,))
                        return "You have successfully joined the group {}.".format(group_name)
        elif action == "like":
            song = request['form']['song']
            if song[-2] == '\\':
                song = song[-2]
            if len(current_users) == 0:
                return "The group {} does not exist.".format(group_name)
            with sqlite3.connect(group_db) as c:
                if (username,) not in current_users:
                    return "You are not in the group {}.".format(group_name)
                # with sqlite3.connect(liked_db) as ccc:
                #     liked_songs = ccc.execute('''SELECT song FROM liked_data WHERE group_name = ?;''',(group_name,)).fetchall()
                #     if (song,) in liked_songs:
                #         return "{} has already been liked in the group {}.".format(song, group_name)
                #     ccc.execute('''INSERT into liked_data VALUES (?,?,?, ?);''',(group_name, user, username, song,))
                #     return "You have successfully added {} to the liked songs in group {}.".format(song, group_name)
                current_users.remove((username,))
                with sqlite3.connect(liked_db) as ccc:
                    for (user,) in current_users:
                        ccc.execute('''INSERT into liked_data VALUES (?,?,?,?);''',(group_name, user, username, song,))
                    return "You have successfully liked {}".format(song)
                    
        elif action == "share":
            song = request['form']['song']
            if len(current_users) == 0:
                return "The group {} does not exist.".format(group_name)
            with sqlite3.connect(group_db) as c:
                if (username,) not in current_users:
                    return "You are not in the group {}.".format(group_name)
                current_users.remove((username,))
                with sqlite3.connect(shared_db) as cccc:
                    for (user,) in current_users:
                        cccc.execute('''INSERT into shared_data VALUES (?,?,?,?);''',(group_name, user, username, song,))
                    return "You have successfully shared {} to the group {}.".format(song, group_name)
        else:
            return "The action {} does not exist.".format(action)