BoxPoker - created by Dave Sawyer
========

Sample C++ application using the Box API and Qt 5.0. This was my 2013 Hackathon project at Box. It's a one day event to build something cool, so my apologies if the code looks a little rushed and not every error condition is handled. OK, hardly ANY error condition is handled. Use at your own risk and write your error handling and do your cleanup so you don't have leaks, etc. A lot of the time you're just looking for *something* to get you off the ground or past some hurdle, and perhaps this will help. This was my first experience with Qt, OAuth2, and fairly low level interaction with the Box V2 API.

BoxPoker is a video poker slot machine. It uses Box to store and share progressive jackpots for the high scoring hands.

Download
========
Windows
  https://cloud.box.com/boxvideopoker
  Grab the whole folder and launch using the .exe within it

Setup of the game
==================
When the game launches, click the "login" button to login to Box. The application uses OAuth2 to authenticate you and connect you to your account.

Select a "table" which is really just a folder on Box that has a jackpots.txt file (there is a sample one in the repo). Use the ID of the folder which is a long number. You can also drop me an email and I'll collab you to a folder with a jackpots.txt file.

Play
====
You start with $1000 as a free gift from the house. Select a bet amount from $10 to $100 and select "Bet!" After you get your cards you can discard 0-5 of them and draw replacements. You win if you have a poker hand of pair of Jacks or better. See wikipedia on hand ranking and probabilities. The better the hand the higher the payoff. The payoff table is shown in the bottom left on the main screen.

Jackpots
========
If you get a really awesome hand, you'll get a payoff PLUS a progressive jackpot. The jackpot amount slowly grows as players lose money to the house. All player's losses feed into the jackpots, so recruit your friends to help build them up! If you bet only $10, you only receive 10% of the possible jackpot, while if you bet the maximum $100 you will receive the entire jackpot. You will also get your name on the winner board as the most recent player to win that jackpot.

If two players win the same jackpot at the same time, the first one to connect through will get the jackpot, the second player will collect from what's left (or $0 if the first player had bet $100).

Tech Stuff
==========
Ok, so you came here not because you crave some video poker, but because you have a project you need to complete. That's fine, you can loot this code to your heart's content with no restriction or accreditation for any purpose, be it non-profit, commercial, or nefarious. Let's get to it!

Check out the other documents to find out more:
Login Information.txt - OAuth2 and refreshing

