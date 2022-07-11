Run the zip file in Eclipse:
copy the zip file to folder eclipse work space
File -> Import
Expand General -> Existing Projects into Workspace -> next
Select Archive File and choose the zip file
-> finish

If you cant see the Project Explorer, close the tab "welcome"
close the tab "donate"
build project
run configuration
Parallel Application
Target System Configuration: MPICH2
Connection Type: Local

According to my text file, number of processes is 4
(It will work with any text file)

Application ->Project -> Browse -> HW02
Application -> Application program -> Browse -> Debug -> HW02 -> open
run

Output:
If the substring found, process 0 print all strings
else, process 0 print: "The string was not found"