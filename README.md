SYNOPSIS
========

Invocation is a system that runs scripts and processes requiring elevated permissions. Unlike sudo it does not depend on setuid, but instead utilizes a client/server system. Invocation has no means of running general commands, one can only configure specific commands against a name, and when that name is called those commands are run. Invocation has additional features that support running these commands as certain users, with lockfiles or pidfiles, or jailing them in chroot containers. It also has features to do extended checks against the process that is calling for the commands to be run, and against the script/command that is being asked to be run.


USAGE
======

Invocation uses the 'peer creds' system for unix sockets. So the requesting program doesn't tell the server what user, group, or program it is, as it could obviously lie about all that. Instead the operating system passes information about the process at the other end of the client-server unix socket connection.

The invocation system consists of two parts: the server 'invoked' and a client process 'invoke'. The protocol spoken between these two is fairly simple, so it can easily be integrated into any code that has support for unix sockets, but for most purposes use of the command-line 'invoke' command will suffice. The standard-output of the run commands is passed to the client, and the 'invoke' command writes that to it's standard output, allowing data to be transferred by this method. Unfortunately, for a number of reasons, std-error output is not currently handled like this.

The 'invoke' command can be passed parameters that can be used in the server-side scripts, like so:

```
	invoke wifi-join essid='mywifi' password='my secret'
```

These parameters can be used on the server side like so:

```
	wifi-join
	{
		run "/sbin/iwconfig wlan0 essid $(essid) key $(password)"
	}
```


INVOKED CONFIG FILE
===================

The invoked server is configured with the 'invoke.conf' config file that normally resides in /etc/. This file consists of a number of entries in the format:

```
<name>
{
<requirements>
<run>
}
```

So, for example:

```
serverbackup
{
user backup_user
require directory /home/Software
run "/bin/tar --exclude=.git -zcO /home/Software"
}
```

In this example the request name is 'serverbackup'. There are two requirements: Only the user 'backup_user' can run this command, and the directory /home/Software must exist for the command to run. The command that is run is tar, which tarballs /home/Software and writes it to it's standard-out. On the client end we would call:

```
invoke serverbackup | tar -zxf -
```

and thus /home/Software would be transferred to the current directory. This is particularly useful when this command is run over ssh, like this:

```
ssh myserver "invoke serverbackup" | tar -zxf -
```

The following 'requirements' entries are supported. At least one of 'user', 'group' or 'program' must be supplied.

```
user <username list>           requestor must be one of the users in this comma-separated list.
group <group list>             requestor must be this group must be in this comma-separated list.
program <program path list>    requestor must be the one of the programs in this comma-separated list.
auth <auth types>              requestor must authenticate using one of auth types.
require absent <path>          nothing must exist at <path> (flag-files)
require exists <path>          something must exist at <path>
require directory <path>       a directory must exist at <path>
require peer-sha256 <hash>     the requestor program file must hash to sha256 <hash>
```

The 'user', 'group' and 'program' commands accept comma-separated-lists. Like so:

```
user adam,eve
group admins,developers
program /usr/bin/vim,/usr/bin/emacs
```

The 'auth' config requires the user to authenticate using linux Pluggable Authentication Modules. The 'auth types' argument is a list of PAM services or configs (usually configured with files in /etc/pam.d) that can be used for authentication. An auth type of '*' means 'any authentication type'. This is not normally needed as the 'peer creds' system already confirms the remote user's identity, but it could be used to implement a secondary password for certain commands.


INVOKED CONFIG FILE: RUN COMMAND
================================

The run command can take some arguments

```
pty                     run command within a pseudo-terminal. Sometimes needed for commands that expect to be talking to a screen and keyboard directly.
timeout=<centisecs>     stop waiting for command to exit after specified number of centisecs (default is wait forever)
expect=<name>           run an 'expect' script that talks to the launched command
user=<user name>        run command as user.
group=<group name>      run command as group.
dir=<path>              run command in directory 'path'.
chroot=<path>           chroot to 'path' before running process.
PidFile=<path>          write the commands process-id to a file at <path>.
LockFile=<path>         create a lockfile, held by the running process, at <path>.
LockStdin=<path>        this creates a lockfile and binds it to the process's standard-in. This is for processes that close any inherited files and don't read anything from standard-in.
files=<value>           limit the number of files the command can have open to <value>.
fsize=<value>           limit any files written to by command to <value> bytes.
mem=<value>             limit memory to <value>.
coredumps=<value>       limit any coredumps written by command to <value> bytes.
nice=<nice value>       set the 'nice' priority of the command.
prio=<nice value>       set the 'nice' priority of the command.
priority=<nice value>   set the 'nice' priority of the command.
```

The fsize and mem arguments can take a 'k', 'M' or 'G' suffix for 'kilo', 'mega' or 'giga' bytes.

so for example a config for mounting an encrypted partition looks like:

```
cryptmount
{
user wwwrun

run "/usr/sbin/cryptsetup open /dev/sda2 crypted.partition" pty expect=Convo timeout=800
run "/bin/mount /dev/mapper/crypted.partition /home/"

Convo
{
expect "Enter passphrase for " "$(PASSPHRASE)"
fail "No key available with this passphrase."
}
}  
```


EXPECT SYSTEM
=============

Invoked supports an expect/chat system that allows the server to talk to the commands it launches. This consists of a list of strings it expects to read from the command, along with a response that it can send in reply.

The expected strings and their replies are specified in the server config file in the format:

```
expect <expected string> <response>
```

 For example:

```
expect "enter username" "$(USERNAME)"
expect "enter password" "$(PASSWORD)"
```

Sometimes it is necessary to detect a string that means the command has failed. This is specified in the config file by a 'fail' entry, like so:

```
fail <expected string>
```


BOOK OUT/IN SYSTEM
==================

The book out/in system allows files to be copied to the client, worked on, and then copied back into place. This means that at no point does the client need to elevate their permissions just to edit a file. It is configured in the server config system like so:

```
bookout /etc/hosts
clientrun /usr/bin/vi ./hosts
bookin /etc/hosts

```

the 'bookout' config sends the specified file to the client. The 'clientrun' config then runs a program *at the client end* (in the above example vi is run against the booked out version of /etc/hosts). Finally, the 'bookin' config sends the altered file back to the server which moves it into /etc/hosts. Both the 'bookout' and 'bookin' commands assume that a booked out file is in the current working directory of the client.


