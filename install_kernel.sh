time sudo make -j 16 modules_install
time sudo make -j 16 install
sudo grub2-mkconfig -o /boot/grub2/grub.cfg
