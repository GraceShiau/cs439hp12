Enable busmastering. (done in pushed code)

Create file runqemu-ifup and scripts (thing my dad sent) -- no longer need sudo for qemu
1000 1000 3 /


//might be necessary for the script
install tuctl (uml-utilities)
change sbin

//To see data we are sending to QEMU
tcpdump -xx -e -i tap0

ping  -c1 192.168.7.2

//To test network configuration, run this image
 sudo qemu-system-x86_64 -net nic,model=rtl8139 -net tap,ifname=tap0 --serial mon:stdio -hda ~/Downloads/linux-0.2.img

//Current best run command for our kernel
qemu-system-x86_64 -net nic,model=rtl8139 -net tap,ifname=tap0 --serial mon:stdio -hdc kernel/kernel.img -hdd fat439/user.img

To skip ARP request (to see actual data packet immediately -- eventually our kernel must do this step):

arp -a (to see mac address cache)
sudo arp -s <address> <max>
