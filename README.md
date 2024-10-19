# Scattergories

## About

Scattergories is a popular pen-and-paper word game that comprises of 2 phases - the answering phase and the voting phase. 
During the *answering phase*, players have to write a logical response to each of the randomly chosen categories that starts with the randomly chosen letter of the round.
During the *voting phase*, for each category, players have to vote either a "yes" or a "no" for all of the other responses based on how logical they are (or any other criteria the house agrees on). 
If there are any similar answers, all of them will be manually invalidated.
All players who have more or the same number of "yes"es as compared to "no"es will receive a point.
The player who has the most points after all categories have been voted on is the winner of that round.

## Usage

### Building

Clone this repo or download it as a zip file and build it with (preferably) Visual Studio.
Do note that this application is only written for Windows.
Once built, do not run the application as the directory layout was not structured with the intention of being able to be run in an IDE.
Instead, look for the generated exe that would be in the x64/Release folder if you built with Visual Studio and run that.

### Running

Select 1 to host a game and enter the same IPv4 address you see in the new console window.
Enter a name and wait for another player to join.
On another computer, run the exe and select 2 to join a game.
Enter the same IPv4 address that you had entered previously (which will be different from the one you will see in this window) and enter a name like before.

## Gameplay

### Answering Phase

During this phase, your current category will be highlighted in blue. Enter an answer to submit a response for that category, which will cause the highlight to move to the next unanswered or invalid category.
You can enter "/w" to scroll the highlight 1 category up or "/s" to scroll it 1 category down.
Valid responses are coloured green while invalid responses are coloured red and will not be sent to the server when this phase ends.

### Voting Phase

During this phase, the response you are currently voting for will be highlighted in blue.
You may use the same slash commands to scroll the highlight, though it would of course skip your own response **(this has not been tested yet)**.
Enter a number between -2 and 2 to cast your vote and add it to the sum of votes for that response.
When the leader goes to the next category, all players with responses that have a sum of votes of more than 0 will receive a point.
As the leader, you may proceed by entering "/n". 
Once all categories have been cycled through, the game ends and the scores will be displayed in the display console.

## Features

### LAN Multiplayer

While not technically online, two or more players can play together if they are connected to the same Wi-Fi network.
Some antivirus software may treat the exe (which is not provided in this repo) as a virus, so it might be best to send the person you want to play with the source code instead.
Be sure to allow this application to bypass any firewalls you may have too!

### Display Console

The display console displays up-to-date information about the game like the players in the game, the amount of time left to answer, and the responses of all the players.

### Client Console

The client console (which is also known as the "player console" in the code) displays the options the player can select as well as other local information like the responses the player has currently submitted.

### Leadership

A leader is able to edit and view the game's settings as well as terminate the game and renounce their leadership.
The first person to join the game will always be chosen as the leader.

### Game Settings

The following can be adjusted by the leader

- Number of questions: 3 - 18 (defaulted to 12)
- Duration: 30 - 180 (defaulted to 120)
- Visibility (whether names are shown during the voting phase): yes, no (defaulted to yes)

## Known Bugs

### Syncing Issues

The display console may sometimes show incorrect information due to network congestion, which is why there's an option in the home page to relaunch it.
The display console may also occasionally fail to connect.
The application may crash when it's trying to start a game (though quite rarely) when it doesn't receive the necessary packets to load the game (I am using UDP after all...for some reason).

### Lack of Testing

I have only tested this application for 2 players as I only have 2 computers, so there may be bugs when more players join.
