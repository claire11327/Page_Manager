# Page_Manager

## Program
### function
#### main
- goal: 
    - get parameter(ex. amount of physical frame...)
    - get instruction from input.txt
#### FIFO_manager
- goal:
    - arrange virtual page and physical frame in FIFO rule
#### ESCA_manager
- goal:
    - arrange virtual page and physical frame in ESCA rule
- rule(ESCA):
    - varable set: (reference & dirty bit)
        - if instruction is "Read" > dir = '0'; else is '1'
        - when the virtual page is called, ref = '1'
        - each time while process is searching  a free frame to store, 
            each node's ref passed through is clear
    - priority: (ref, dir)
        - the first state to be replace :[0, 0] > [0, 1] > [1, 0] > [1, 1]
#### SLRU_manager
- goal:
    - arrange virtual page and physical frame in SLRU rule
- rule(SLRU):
    - varable set: (reference & activate bit)
        - when the virtual page is in physical memory with ref == '0' and then be called again, set ref
        - when the virtual page is in physical memory with ref == '1' and then be called again, set act
            (means this vitual page is now in active list)
    - change list: (if called)
        - if ref == '1' && act == '0' , move to the head of active list
        - if ref == '1' && act == '1' , move to the head of active list
        - if active list is full, move the tail of active list to the head of inactive list
    - priority: 
        - FIFO(remove from the tail of inactive list)

## Input File Format
```
    1 Policy: FIFO | ESCA | SLRU
    2 Number of Virtual Page: M
    3 Number of Physical Frame: N
    4 -----Trace-----
    5 Read 1
    ...
```

## Output File Format
- Hit, VPI=>PFI
- Miss, PFI, Evicted VPI>>Destination, VPI<<Source
- Page fault rate: Rate

