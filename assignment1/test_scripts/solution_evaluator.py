
#!/usr/bin/env python

import csv
import os
import re
import argparse


parser = argparse.ArgumentParser()
parser.add_argument('--file', help='Enter the full Path of the output file',
                    nargs='?', default="", const="")
parser.add_argument(
    '--nProducers', help='Enter number of producer threads', nargs='?', default=1, const=1)
parser.add_argument(
    '--nConsumers', help='Enter number of consumer threads', nargs='?', default=1, const=1)
parser.add_argument('--nItems', help='Enter number of items',
                    nargs='?', default=1, const=1)

args = parser.parse_args()
inputFilePath = args.file
nProducers = int(args.nProducers)
nConsumers = int(args.nConsumers)
nItems = int(args.nItems)
expectedResult = nItems
expectedResultValue = (nItems * (nItems - 1)) / 2 #Since itemCount starts from 0, nItems-1 is the last element in the series

consumer_string = "^consumer_id, items_consumed_type0:value_type0, items_consumed_type1:value_type1, items_consumed_type2:value_type2, time_taken"
#for p in range(nProducers):
#    consumer_string = consumer_string + ", value_from_producer_%s" %p

spacePattern = "(\s)*"
numberOfItemsPattern = re.compile(
    "^Number of items:{}(\d+)".format(spacePattern, spacePattern))
numberOfProducersPattern = re.compile(
    "^Number of producers:{}(\d+)".format(spacePattern, spacePattern))
numberOfConsumersPattern = re.compile(
    "^Number of consumers:{}(\d+)".format(spacePattern, spacePattern))
#queueSizePattern = re.compile(
#    "^Queue size:{}(\d+)".format(spacePattern, spacePattern))
producerThreadStatsPattern = re.compile(
    "^producer_id, items_produced_type0:value_type0, items_produced_type1:value_type1, items_produced_type2:value_type2, total_value_produced, time_taken")
consumerThreadStatsPattern = re.compile(consumer_string)
#    "^consumer_id, items_consumed, value_consumed, time_taken")
totalProducedPattern = re.compile(
    "^Total produced{}={}(\d+)".format(spacePattern, spacePattern))
totalProducedValuePattern = re.compile(
    "^Total value produced{}={}(\d+)".format(spacePattern, spacePattern))
totalConsumedPattern = re.compile(
    "^Total consumed{}={}(\d+)".format(spacePattern, spacePattern))
totalConsumedValuePattern = re.compile(
    "^Total value consumed{}={}(\d+)".format(spacePattern, spacePattern))
timeTakenPattern = re.compile(
    "^Time taken \(in seconds\):{}(\d*\.\d*)".format(spacePattern, spacePattern))


startProcessingProducerData = False
startProcessingConsumerData = False
finishedProcessingProducerData = False
finishedProcessingConsumerData = False

processingOverallData = False
numberOfColumns = 6

producerData = list()
consumerData = list()

nProducersRead = 0
nConsumerssRead = 0

totalProduced = 0
totalProducedValue = 0
totalConsumed = 0
totalConsumedValue = 0
totalTime = 0.0

consoleLogContents = ""
if(inputFilePath != ""):
    with open(inputFilePath, 'r') as inputFile:
        consoleLogContents = inputFile.read()

consoleLogContents = consoleLogContents.split('\n')

### Parse the output logs ###
for line in consoleLogContents:
    if(startProcessingProducerData == False):
        if(numberOfProducersPattern.match(line)):
            lineSplit = line.split(':')
            lastWord = lineSplit[-1].strip()
            nProducersRead = int(lastWord)
            continue
        if(numberOfConsumersPattern.match(line)):
            lineSplit = line.split(':')
            lastWord = lineSplit[-1].strip()
            nConsumerssRead = int(lastWord)
            continue
        if(producerThreadStatsPattern.match(line)):
            startProcessingProducerData = True
        continue

    # // Can proceed
    if(finishedProcessingProducerData == False):
        lineSplit = line.split(',')
        if(len(lineSplit) == numberOfColumns):
            threadID = lineSplit[0].strip()
            itemsProduced0 = lineSplit[1].strip()
            itemsP_split = itemsProduced0.split(':')
            itemsP_count0 = itemsP_split[0]
            itemsP_value0 = itemsP_split[1]
            itemsProduced1 = lineSplit[2].strip()
            itemsP_split = itemsProduced1.split(':')
            itemsP_count1 = itemsP_split[0]
            itemsP_value1 = itemsP_split[1]
            itemsProduced2 = lineSplit[3].strip()
            itemsP_split = itemsProduced2.split(':')
            itemsP_count2 = itemsP_split[0]
            itemsP_value2 = itemsP_split[1]
            valueProduced = lineSplit[4].strip()
            timeTaken = lineSplit[5].strip()
            producerData.append(
                (threadID, int(itemsP_count0), int(itemsP_value0), int(itemsP_count1), int(itemsP_value1), int(itemsP_count2), int(itemsP_value2), int(valueProduced), float(timeTaken)))
            continue
        else:
            finishedProcessingProducerData = True

    if startProcessingConsumerData == False:
        if(totalProducedPattern.match(line)):
            lineSplit = line.split('=')
            lastWord = lineSplit[-1].strip()
            totalProduced = int(lastWord)
            continue
        if(totalProducedValuePattern.match(line)):
            lineSplit = line.split('=')
            lastWord = lineSplit[-1].strip()
            totalProducedValue = int(lastWord)
            continue
        if(consumerThreadStatsPattern.match(line)):
            startProcessingConsumerData = True
        continue

    if(finishedProcessingConsumerData == False):
        lineSplit = line.split(',')
        if(len(lineSplit) == (numberOfColumns-1)):
            threadID = lineSplit[0].strip()
            itemsConsumed0 = lineSplit[1].strip()
            itemsC_split = itemsConsumed0.split(':')
            itemsC_count0 = itemsC_split[0]
            itemsC_value0 = itemsC_split[1]
            itemsConsumed1 = lineSplit[2].strip()
            itemsC_split = itemsConsumed1.split(':')
            itemsC_count1 = itemsC_split[0]
            itemsC_value1 = itemsC_split[1]
            itemsConsumed2 = lineSplit[3].strip()
            itemsC_split = itemsConsumed2.split(':')
            itemsC_count2 = itemsC_split[0]
            itemsC_value2 = itemsC_split[1]
#            valueConsumed = lineSplit[2].strip()
            timeTaken = lineSplit[4].strip()
            consumerDataLine = [threadID, int(itemsC_count0), int(itemsC_value0), int(itemsC_count1), int(itemsC_value1), int(itemsC_count2), int(itemsC_value2), float(timeTaken)]
            #append(
            #    (threadID, int(itemsConsumed), int(valueConsumed), float(timeTaken)))
#            for p in range(nProducers):
#                consumerDataLine.append(int(lineSplit[4+p].strip()))
            consumerData.append(consumerDataLine)
            continue
        else:
            finishedProcessingConsumerData = True

    # Process overall data
    if(totalConsumedPattern.match(line)):
        m = totalConsumedPattern.search(line)
        l = len(m.groups())
        totalConsumed = int(m.group(l))
    if(totalConsumedValuePattern.match(line)):
        m = totalConsumedValuePattern.search(line)
        l = len(m.groups())
        totalConsumedValue = int(m.group(l))
    if(timeTakenPattern.match(line)):
        m = timeTakenPattern.search(line)
        l = len(m.groups())
        totalTime = float(m.group(l))


### Validate all information is correctly parsed ###

# print("Parsed information from the program execution")
# print("Total points generated : {}".format(totalPoints))
# print("Total points in circle : {}".format(totalCirclePoints))
# print("Result : {}".format(result))
# print("Time taken (in seconds) : {}".format(totalTime))
# print("Time taken (in seconds) : {}".format(result))

validationFlag = True
if nProducersRead != nProducers:
    validationFlag = False
    print("VALIDATION FAILED: Incorrect number of producers used = {}".format(
        nProducersRead))

if nConsumerssRead != nConsumerssRead:
    validationFlag = False
    print("VALIDATION FAILED: Incorrect number of consumers used = {}".format(
        nConsumerssRead))

if totalProduced == 0:
    validationFlag = False
    print("VALIDATION FAILED: Total items produced = {}".format(totalProduced))

if totalProducedValue == 0:
    validationFlag = False
    print("VALIDATION FAILED: Total value of items produced = {}".format(totalProducedValue))

if totalConsumed == 0:
    validationFlag = False
    print("VALIDATION FAILED: Total items consumed = {}".format(totalConsumed))

if totalConsumedValue == 0:
    validationFlag = False
    print("VALIDATION FAILED: Total value of items consumed = {}".format(totalConsumedValue))

# if totalTime < 0.000000000000000001:
#     validationFlag = False
#     print("VALIDATION FAILED: Time taken = {}".format(totalTime))


if nProducers != len(producerData):
    validationFlag = False
    print("VALIDATION FAILED: Thread statistics only found for {} producers".format(
        len(producerData)))

if nConsumers != len(consumerData):
    validationFlag = False
    print("VALIDATION FAILED: Thread statistics only found for {} consumers".format(
        len(consumerData)))

for value in producerData:
    if len(value) != 9:
        validationFlag = False
        print("VALIDATION FAILED: Incorrect format for producer statistics logs\n Expected format : {}".format(
            producerThreadStatsPattern.pattern))

for value in consumerData:
    if len(value) != 8:
        validationFlag = False
        print("VALIDATION FAILED: Incorrect format for consumer statistics logs\n Expected format : {}".format(
            consumerThreadStatsPattern.pattern))

if validationFlag == True:
    print("Validation successful")
else:
    print("Validation failed")
    exit(1)


### Evaluation of the processed information ###

minItemsConsumed = 999999999999
maxItemsConsumed = 0
minTimeTaken = 99999999999.0
maxTimeTaken = 0.0
totalItemsConsumedByThreads = 0
totalValueOfItemsConsumedByThreads = 0
value_by_type=[]
valueP_by_type=[]
for t in range(3):
    value_by_type.append(0)
    valueP_by_type.append(0)
for v in consumerData:
    itemsConsumed = v[1]+v[3]+v[5]
    valueConsumed = v[2]+v[4]+v[6]
    timeTaken = v[7]
    for t in range(3):
        value_by_type[t] += v[2*t+2]
    minItemsConsumed = min(minItemsConsumed, itemsConsumed)
    maxItemsConsumed = max(maxItemsConsumed, itemsConsumed)
    minTimeTaken = min(minTimeTaken, timeTaken)
    maxTimeTaken = max(maxTimeTaken, timeTaken)
    totalItemsConsumedByThreads += itemsConsumed
    totalValueOfItemsConsumedByThreads += valueConsumed

minItemsProduced = 999999999999
maxItemsProduced = 0
minTimeTaken = 99999999999.0
maxTimeTaken = 0.0
totalItemsProducedByThreads = 0
totalValueOfItemsProducedByThreads = 0
value_by_type_match = True
p = 0
for v in producerData:
    itemsProduced = v[1]+v[3]+v[5]
    valueProduced = v[2]+v[4]+v[6]
    for t in range(3):
        valueP_by_type[t] += v[2*t+2]
    timeTaken = v[8]
    p  = p+1
    minItemsProduced = min(minItemsProduced, itemsProduced)
    maxItemsProduced = max(maxItemsProduced, itemsProduced)
    minTimeTaken = min(minTimeTaken, timeTaken)
    maxTimeTaken = max(maxTimeTaken, timeTaken)
    totalItemsProducedByThreads += itemsProduced
    totalValueOfItemsProducedByThreads += valueProduced

for t in range(3):
    if (value_by_type[t] != valueP_by_type[t]):
        value_by_type_match = False


successFlag = True

# Validate number of items produced
if(totalItemsProducedByThreads != expectedResult):
    successFlag = False
    print("EVALUATION FAILED : Total number of items produced is incorrect({}). Expected {}.".format(
        totalItemsProducedByThreads, expectedResult))

if(totalValueOfItemsProducedByThreads != expectedResultValue):
    successFlag = False
    print("EVALUATION FAILED : Total value of items produced is incorrect({}). Expected {}.".format(
        totalValueOfItemsProducedByThreads, expectedResultValue))

if (value_by_type_match == False):
    successFlag = False
    print("EVALUATION FAILED : Total value produced for (at least) one type doesn't match total consumers' value for that type")

# Validate number of items consumed
if(totalItemsConsumedByThreads != expectedResult):
    successFlag = False
    print("EVALUATION FAILED : Total number of items consumed is incorrect({}). Expected {}.".format(
        totalItemsConsumedByThreads, expectedResult))

if(totalValueOfItemsConsumedByThreads != expectedResultValue):
    successFlag = False
    print("EVALUATION FAILED : Total value of items consumed is incorrect({}). Expected {}.".format(
        totalValueOfItemsConsumedByThreads, expectedResultValue))


if successFlag == True:
    print("Evaluation successful")
else:
    print("Evaluation failed.")
    print("Ensure that you do not print more data than expected and the expected output format is strictly followed")
