# Arcade Machine — First Progress Report

**First of all, well done.**

This should be the first progress report for the arcade machine. Given the project’s risk of discontinuity and the development difficulties that such discontinuity brings, I am writing this document and sincerely recommend that every future development team come back here to write your summary at the end of each semester.

This semester, we spent a great deal of time accepting the fact of this project’s discontinuity and putting in effort to understand and improve it. Although I believe we did not accomplish much, and I personally feel I did not work hard enough, I still want to provide whatever help I can for future developers. So next I will introduce everything I know and understand, and I hope your development goes smoothly.

## What is the arcade machine?

The arcade machine is an entertainment machine developed by Deakin students and placed across Deakin campuses. As of September 25, 2025, the status is: the units currently on campus can only play three games (about one quarter of what’s in the arcade-games Git repository). Various settings and buttons are missing, so the machine functions only as a device for playing games, and the player experience is not ideal. That said, given the pandemic context, the previous developers building this from zero to one was already a remarkable achievement.

## How is the arcade machine going?

In fact, the arcade machine consists of two Git repositories—“arcade machine” and “arcade games.” At present, its situation may not be ideal. Based on the arcade machine and arcade games code, we set up two test rigs (one on RPi 5, one on RPi 3B). However, the version running on the real arcade machine differs completely from the version built from the repositories. I do not currently know why there is such a large discrepancy, nor do I know the exact source code base used to build the real machine. Therefore, I cannot tell you with certainty whether the code and logic are the same between the two.

Based on certain facts (the three games that run on the real arcade are a subset of the arcade-games repository; a previous developer told me that, due to image copyright considerations, they did not use the version from the repo), I can only offer my personal judgment: the code environment on the real machine should be broadly similar to what’s in the repositories, but perhaps due to build issues or other reasons, only three games from arcade-games are present there.

## What did our team do this semester?

- Contributed three games to arcade-games:  
  - a tower defense game (SDL2 engine),  
  - a 2D RPG (Unity engine), and  
  - Tetris.
- Wrote explanatory documentation in many places across both the machine and games repositories.
- Prepared two test rigs: one RPi 5 and one RPi 3B.
- Authored this progress report.

I am ashamed that our team did not do enough, so I wrote this report to provide help until the very last moment.

## Points to note for the arcade machine

For now, there are not many special caveats. You can find answers to most questions in the explanatory documents I placed throughout the repositories. The machine repository itself is already relatively complete and does not have any particularly unusual pitfalls.

One thing worth mentioning is the arcade games side. The repository’s automatic CI will automatically pull the latest updates from games, compile them, and put everything into a compiled games folder. This seems like a great design that would allow automatic updates of games on the arcade machine. However, in the arcade machine’s game-pulling logic, it first looks for the execution path in config, then looks for builds, and if it still cannot find anything, it fails. As a result, the compiled games folder is not actually used in this process. I ultimately did not fully understand the purpose of this design; perhaps it serves as a ready-to-use backup repository of pre-compiled games.

Another important matter: while developing the machine test rigs, we encountered version incompatibilities with the SplashKit framework. The SplashKit used by developers three years ago is quite different from today’s. Using the latest version leads to compatibility issues. I did not dare to undertake a full repository refactor lightly, so in the test environment I used a SplashKit version prior to 2022.8.21 to fully reproduce the earlier arcade environment.

## Closing

I sincerely hope this document helps future developers. When you try to do more for the arcade, getting your hands on the test rigs and the repository documents—together with this report and the notes scattered across the repos—should improve your development experience. I’m sorry I couldn’t do more for you. If you would like to know more details about our development at the time, feel free to email me; I will do my best to help: 903157209hy@gmail.com.

Wishing you happy times with the arcade machine!

---

September 25, 2025  
Haoyu Liu  
Arcade Machine 2025 T3 Development Team
