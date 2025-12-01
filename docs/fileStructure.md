# File Structure & Architectural Motives

Currently, this repo is split into numerous directories/files based on functionality. The motivation behind this is to make everything more modular and aid in debugging and developer experience. File size should ultimately be minimized.

Below, each directory's purposes will be briefly explained.

## `/data-generation`

Focuses on holding the current csv file to test and the python script used to generate said data.

## `/docs`

Documentation and reports. Used mostly for developer experience and to help explain any confusing decisions.

## `/engine`

This serves as the **main powerhouse** of the program. This holds the B+ tree implementation (*bplus-x.c*), utility functions for *building* the different trees that will be used for indexing (*buildEngine-x.c*), and various functions that can be used for executing specific commands (*executeEngine-x.c*). The execute file will hold the specific commands for things such as SELECT, WHERE, INSERT, DELETE, etc. This should be used as a means to abstract the lower-level functionality to be used in the root-level files (*QPEx.c*). For more of an explanation of how this will work, see the **bplus.md** md file in the docs folder.

## `/include`

This will hold **all** header files, used as a centralized way to use different utilities and whatnot across all directories. This will just make importing and stuff easier.

## `/tests`

Any basic tests ran during development to verify the functionality of any utilities.

## `/tokenizer`

Given a string SQL command, will parse it to determine the actual functionality desired by the user.

## `QPE.c`

The *QPEMPI.c*, *QPEOMP.c*, and *QPESeq.c* uses the wrapper functions in the `/engine` directory to perform high-level queries. For now, these files should read in each command in the `sample-queries.txt` file, use the parser to determine which specific functionality is desired, then run it through the engine to get the specific results. This will also perform high-level benchmarking.