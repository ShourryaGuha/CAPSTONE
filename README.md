# CAPSTONE

For uwaterloo capstone 2025. 

------Raspberry pi login details-----
Hostname: bombsite.local (this isn't always working, ip address is consistently 192.168.91.50)
Username: shourryag
Password: mte482@caps


------How to SSH into RPI------
NOTE: You need to be connected to the SAME network, or at least have a vpn so that you have the same IP (192.168.91.50)
ssh does not work from different networks by default. Need to enable port forwarding on connected router,
and since it's my phone's mobile hotspot, I'm not sure that it's possible. 

Now, open a git bash terminal:

ssh shourryag@192.168.91.50
OR
ssh shourryag@bombsite.local

answer yes to the prompt << Are you sure you want to continue connecting (yes/no/[fingerprint])? >>
Then type in the password on prompt

then, in a new terminal, navigate to ~/.ssh/ (or wherever your ssh keys are stored) and type in:

ssh-copy-id -i <<ssh public filename>> shourryag@192.168.91.50
for example:
ssh-copy-id -i id_ed25519.pub shourryag@192.168.91.50

This will allow you to work from the same computer without always having to put in a password. 

---------FILE TRANSFER------------
If you want to manually transfer files for some reason, like you want to do a quick test, or whatever:
To the remote host (the rpi) from local:

scp -r CAPSTONE/ shourryag@192.168.91.50:~/Documents/FILE_TRANSFER_ONLY/

And if you want to transfer a file to local:

scp -r shourryag@192.168.91.50:~/Documents/FILE_TRANSFER_ONLY/ CAPSTONE/

Otherwise, please just use the github repo