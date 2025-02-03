Mikamp and the Mikmod Sound System v3.1.0 

A historical account, from 1994 to 2003-ish.
As written by Jake Stine.

Glossary:
 - Mikamp - a fork of the Mikmod Sound System specialized for use with Winamp.


## Ancient History: 1993 to 1998-ish

Mikmod was originally created by Jean-Paul Mikkers, and was originally intended to serve a very specific
purpose: to play mod/s3m files on the newly released Gravis Ultrasound soundcard. The project was a
success and Mikmod quickly gained popularity and notoriety as being one of the first and best module
players for Gravis Ultrasound owners.

User requests and high ambitions resulted in a continual expansion of Mikmod features. Mikkers added XM
format support, Soundblaster support, and a software mixer capable of playing music in the background
of MS-DOS. Paul Fisher and I worked with him as interface providers – we designed and coded a UI
frontend that Paul named **Extended Mikmod** (XMM). It's hallmark traits were:
 - a very clever heuristic that pulled composer names from the instruments lists
 - a very clever swipe-based user interface that, 20-25 yrs before tindr or tiktok, was well ahead of
   its time
 - a lot of nasty hacks, memory leaks, unstable behaviors, and hard crashes

Eventually Jean-Paul moved on to better things. He left Mikmod in my care to do with it as I pleased.
For a while I dabbled with implementing some low-hanging fruit in XM format support, and then at the
request of some composers (namely Mathew Valente), I agreed to add support for the newly developed
**ImpulseTracker** (IT) module format. Impluse Tracker's highly advanced feature set, coupled with
Jeffrey Lim's relative lack of file format planning or clear documentation, turned what seemed like a
managable task into a years-long adventure.

Supporting the gambit of ImpulseTracker's features eventually required that I refactor a significant
portion of Mikmod's internals. Mokmod as a module player had been written with simplicity in mind: It
took the known subset of MOD/S3M/XM formats, applied some strong assumptions as guiding development
principles, and surfaced a mostly data-driven API on which to load the sample and musical pattern
data. ImpulseTracker broke nearly all of these assumptions. The resulting version of Mikmod was mostly
program-driven, with most data-driven aspects removed.

### Unimod Format

There was one core concept from Mikmod that survived: The Unimod Module Format.

I made some special effort to keep one of Jean-Paul's original design goal for Mikmod: a unified module
format called **Unimod**. The Unimod format used a bytecode system similar in design to intermediate
representations (ILs) used by for interpreted or JIT-compiled languages like Java, Javascript, C#, etc.
Unimod provided a mid-level accelerated medium between high-level module formats (which are typically
oriented for use by module trackers), and the low-level playback routines needed to produce the final
musical results. The early versions of Unimod were very high-level; basically pre-compiled instances of
MOD/S3M module effects. Mikkers started adding XM support to Unimod and found it to be very cumbersome,
and that experience discouraged him entirely from even attempting to add ImpulseTracker support.

But I saw a potential path forward. First, I decided to restrict Unimod to an _internal representation_
concept only, and removed the requirement to maintain any sort of serialization or deserialization of
the format. This freed me up to move laterally with major intenral refactoring of the internal structure
without worrying about what might also work well at the serialization level. Next, I scrapped the old
Unimod design and re-built a new Unimod using a much lower level bytecode design. A reasonable analogy
would be RISC vs. CISC style instruction sets: the new Unimod was RISC-style, with it's internal
representation using series of bytecode instructions combined together to build more complex musical
effects behaviors that the various module formats needed.

Roughly 90% of all module formats, including ImpulseTracker, can be accurately represented by Unimod's
core RISC-style module instruction set. The other 10% is dealt with via specialized commands or
parameters. I felt satisfied in having exceeded the 80/20 rule. The bulk of the Unimod core was well-
defined and easier to verify, and the module-specific hacks were easier to implement without impacting
code which was known to be operating correctly.

A surprising benefit of my new Unimod design was playback efficiency, especially for IT modules. Much
of the overhead in ImpulseTracker replay is in managing the complex effects and virtual voice allocation
system. The pre-compiled bytecode stream allowed me to encode a bunch of useful information during
analysis stages. The result was that Mikamp was ***by far*** the most efficient IT player available at
the time. This served it well for the select few games that Mikmod was featured in.

### Linux Porting and Public Domain

By this time the date was around 1997. I made some effort to try and monetize Mikmod around that time
but that went nowhere. I was having trouble keeping up with ImpulseTracker's constant updates sporting
major changes to its format. I shifted gears to focus on indie game development and that motivated me
to add game friendly APIs to Mikmod for triggering and managing sound effects playback, and also to port
the library to Linux as it had a budding indie gamedev scene at the time (largely thanks to the release
of SDL 1.2).

I decided to give up on monetization and instead relinquish Mikmod to the public domain. From that point
it became a popular module player on Linux thanks to it's simple-to-compile C codebase and my efforts to
add portable endpoints. I retained my own personal fork of Mikmod for use in indie gamedev projects.

## Winamp Integration History [Mikamp] (1998-2001)

The **Mikamp Plugin** was born some time in 1998 or 1999, only a few months after I had relinquished
Mikmod to what was effectively public domain. I was contracted by Nullsoft to provide a module music
plugin for Winamp v2. I agreed. With Mikmod back in the public eye, I got renewed interest to implement
ImpulseTracker features that I had left on the table a year ot two prior, such as Resonant Filters.

I also had an idea to leverage the lightning fast Unimod IR to implement realtime seeking. At the time
module players had only supported seeking on a pattern level of granularity, which was several seconds
in length or longer, and had unwanted side effects such as missing bakcground instruments that might
have been playing across patterns. The Unimod IR allowed me to do near instant seeking to any point in
a module from the starting position. Final reasult: Mikamp's thumb seek was nearly as seamless as an
MP3 seek.

I discontinued work on the public Mikamp Plugin sometime in 2000 or 2001.

## Jan Lönnberg Contributions (2000-2001)

In late 2000 or early 2001 I granted CVS repository access to Jan Lönnberg. Jan proceeded to fix
dozens of bugs in Mikamp's module support. A complete list of his work is still documented in 
`BUGLIST.txt`. I had fully committed myself to indie gamedev during that time and was unaware of
the work until Q1 in 2005, which was more than a year after the CVS repository had been taken down.

Thanks to Jan's work, Mikamp's accuracy as a module music sound system was probably unparalleled for
the next 10 years or more, barring the actual trackers for IT/XM/S3M formats. Unfortunately the work
fell into obscurity.

## Re-release attempts in 2005 and 2010

I made an effort in 2005 to re-release Mikamp publicly, expecting to post it to SourceForge, with
the goal of bringing the high quality player out of obscurity. That fell by the wayside. In 2010 I
made another attempt, this time posting Mikamp to GoogleCode without any website to help promote it.
The project never attracted attention and a couple years later GoogleCode itself was mothballed by
Google.

The release posted to GoogleCode in 2010 was done so under the GNU GPL v3. This choice of license
was made somewhat arbitrarily based on the popularity of the license at the time among other FOSS
projects to which I was contributing, and I thought the choice of license might serve the purpose
of increasing the potential chance for contributors or exposure of the project. In that regard,
it failed.

## Re-release onto Github (2025)

Fifteen years have passed. Here we go again. This time around I'm moving the license back into
the unencumbered public domain, and erasing my prior mistake. Adoption of the project is far
less interesting than the historical archival record.

