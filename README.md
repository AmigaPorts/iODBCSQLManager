
SQLManager for AmigaOS4 and iODBC library
======================

What is ODBC? 
-----

ODBC is the acronym for Open DataBase Connectivity, a Microsoft Universal Data Access standard that started life as the Windows implementation of the X/Open SQL Call Level Interface specification. Since its inception in 1992 it has rapidly become the industry standard interface for developing database independent applications. Is is also the emerging standard interface for SQL based database engines replacing many of the first generation Embedded SQL and proprietary call level interfaces provided by database engine and database connectivity middleware vendors alike. What is the ODBC Value Proposition? The ability to develop applications independent of back-end database engine. What is iODBC? iODBC is the acronym for Independent Open DataBase Connectivity, an Open Source platform independent implementation of both the ODBC and X/Open specifications. It is rapidly emerging as the industry standard for developing solutions that are language, platform and database independent. What is the iODBC Value Proposition? The ability to develop applications independent of back-end database engine, operating system, and for the most part programming language. Although ODBC and iODBC are both 'C' based Application Programming Interfaces (APIs) there are numerous cross language hooks and bridges from languages such as: C++, Java, Perl, Python, TCL etc. iODBC has been ported to numerous platforms, including Linux (x86, Itanium, Alpha, Mips, and StrongArm), Solaris (Sparc & x86), AIX, HP-UX (PA-RISC & Itanium), Digital UNIX, Dynix, Generic UNIX 5.4, FreeBSD, MacOS 9, MacOS X, DG-UX, and OpenVMS and now on AmigaOS4. Which drivers are supported? Actually most of common Open Source database are supported. Drivers for MySQL, PostgreSQL and SQLite are present into the package and has been tested connecting AmigaOS4 to a Windows machine. Also SQL Server is supported. Other drivers are in working.

Supported Drivers
-----

- SQL Server - Driver for Microsoft SQL Server / SyBase Database (0.83)
- MySQL - Driver for MySQL Server Database (3.51.27r695)
- PostgreSQL - Driver for PostgreSQL Database (06.40.0007)
- SQLite3 - Driver for SQLite3 Database (0.83)
- MS Access - Driver for Microsoft Access Database
- NN - Driver for News Server access

Main Features of the ODBC Engine
-----

- Fully localized (only English available at moment)
- Package installer
- Connection Wizard
- Inline hint infos

How to build
-----

All you need to compile the project is already included int this repository. You have to just execute make command to build the project

- Execute *make* to build the SQL Manager executable
- Execute *make clean* to clean the project