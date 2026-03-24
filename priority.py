import time
import random

def escalate(category, waiting):

    if category == 4:
        if waiting > 50:
            return 1
        elif waiting > 30:
            return 2
        elif waiting > 15:
            return 3

    elif category == 3:
        if waiting > 40:
            return 1
        elif waiting > 20:
            return 2

    elif category == 2:
        if waiting > 35:
            return 1

    return category


def stabilize(category, waiting):

    # Stabilization probability increases over time
    chance = random.randint(1, 100)

    if waiting > 20:
        if category == 1 and chance < 20:
            return 2   # Red → Orange
        elif category == 2 and chance < 25:
            return 3   # Orange → Yellow
        elif category == 3 and chance < 30:
            return 4   # Yellow → Green

    return category


def calculate_priority(category, waiting, distance):

    # Step 1: Escalation
    category = escalate(category, waiting)

    # Step 2: Stabilization
    category = stabilize(category, waiting)

    # Priority score
    if category == 1:
        base = 100
    elif category == 2:
        base = 80
    elif category == 3:
        base = 60
    else:
        base = 40

    return base + waiting - distance, category


with open("patients.txt", "r") as f:
    lines = f.readlines()

updated = []
current_time = int(time.time())

for line in lines:
    data = line.strip().split()

    name = data[0]
    category, distance, arrival = map(int, data[1:])

    waiting = current_time - arrival

    priority, new_category = calculate_priority(category, waiting, distance)

    updated.append(f"{name} {new_category} {distance} {arrival} {priority}\n")

with open("patients.txt", "w") as f:
    f.writelines(updated)