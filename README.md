# Kerno's C++ Linux Engineering Challenge

## The challenge:
Write a C++ (linux) program "processor" that receives a stream of http "messages" (both inbound and outbound) via unix pipe.
The program maintains a minute by minute aggregation map by **request path** and **response code** dimensions.
Messages will not be ordered (you can receive 4 requests in a row, and subsequent responses), so you need to find a means to correlate them.
Every minute the map should be printed to a file indicated by the `-o <filepath>` argument received by your program.


### What we will be looking at?
- your code does what it's supposed to (duh!)
- your code is well organized and extensible (clean code practices)
- your code leverages adequately language features (C++20 and on)


### Some food for thought (a.k.a., nice to have's)
- What would happen if we added messages in another protocol / format?
- What would happen if the message rate exceeded your CPU capacity?
- What would happen if some messages never got a response?


## Clone this project to get started!
It contains a little message generator binary for linux (run it in a non-privileged docker please) that you can use to get the messages streaming -- mind that you need an x86 arch host:
```bash
$ ./generator | ./processor
```

### Deliverable
Please be kind to provide Dockerfile(s) to build and and test your code... setting C++ environments can be a bit painful and we have a lot of applications to review!
