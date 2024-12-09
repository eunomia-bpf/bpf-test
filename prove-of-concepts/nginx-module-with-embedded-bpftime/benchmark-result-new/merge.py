import os
import json
result={}
for item in os.listdir("."):
    if not item.startswith("merged.json") and item.endswith(".json"):
        with open(item,"r") as f:
           result[item]=json.load(f)
with open("merged.json","w") as f:
    json.dump(result,f) 
