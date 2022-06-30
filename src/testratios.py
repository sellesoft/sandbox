from math import ceil, floor

ratios = [1,1,4,5,6,7]
size = 400


sum = 0
for i in range(len(ratios)):
    sum += ratios[i]

sum2 = 0
for i in range(len(ratios)):
    sum2 += floor(ratios[i] / sum * size)
    print(ratios[i] / sum * size)

print(sum2)

sum3 = 0
for i in range(len(ratios)):
    if i < size - sum2:
        sum3 += ceil(ratios[i]/sum*size)
    else:
        sum3 += floor(ratios[i]/sum*size)

print(sum3)
