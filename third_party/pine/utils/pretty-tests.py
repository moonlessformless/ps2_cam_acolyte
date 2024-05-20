import json
import sys

f=open(sys.argv[1])
y = json.loads(f.read())
print("Tests results: " + str(y["result"])) 
print("Tests duration: " + str(y["duration"])) 
print("Tests output:\n~~~~~~~~~~~~~~~~~~~~\n" + str(y["stdout"])) 
