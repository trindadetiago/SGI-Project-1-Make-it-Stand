import gpytoolbox as gpy
import torch
import polyscope as ps
import matplotlib.pyplot as plt



V = torch.tensor([
    [-2, 0], [-2, 2], [0, 2], [0, 0],
    [2, 0], [2, -2], [0, -2]
])
E=torch.tensor([
    [0, 1], [1, 2], [2, 3], [3, 0],
    [3, 4], [4, 5], [5, 6], [6, 3]
])


line=torch.tensor([1,-0.3]) #r and c that define the line by cos(r)x+sin(r)y=c

r=line[0] % (2*torch.pi) # make r in [0,2pi)
c=line[1]
#normal of the line, orientated
n=torch.tensor([torch.cos(r),torch.sin(r)])
#line in y=mx+b format
m=-torch.cos(r) / (torch.sin(r)+1e-12)
b=c / (torch.sin(r)+1e-12)


#calculate the intersection loop
intersec=torch.empty((1, 2))

for edge in E:
    v0=V[edge[0]]
    v1=V[edge[1]]
    # the line of the edge
    m0 = (v1[1] - v0[1]) / (v1[0] - v0[0]+1e-12)
    b0 = v0[1] - m0 * v0[0]
    #compute intersection
    if m  != m0 and b != b0:
        x = (b0 - b) / (m  - m0+1e-12)
        y = m  * x + b
        i = torch.tensor([[x,y]])
        #check if it is in the edge
        if torch.min(v0[0], v1[0]) - 1e-12 <= x <= torch.max(v0[0], v1[0]) + 1e-12 and torch.min(v0[1], v1[1]) - 1e-12 <= y <= torch.max(v0[1], v1[1]) + 1e-12:
            intersec = torch.cat((intersec, i), dim=0)
            print(edge,"here")
        #else it doesn't is in the edge
intersec=intersec[1:, :]
print(intersec)







