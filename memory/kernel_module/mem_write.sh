phys_addr=0x1100000000
index=0
max=100

while [ $index -le $max ]
do
    let phys_addr=$phys_addr+0x8000
    let index+=1
    `echo "obase=16;$phys_addr"|bc > /sys/kernel/mem_debug/physical_start_address`
    `echo 1 > /sys/kernel/mem_debug/mem_write_request`
done