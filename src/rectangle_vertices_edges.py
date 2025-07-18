def rectangle_vertices_edges(V, E, idx_start, idx_end, c):
	
	next_idx = V.shape[0] # determine the next index based on length of vertex list tensor

	new_V = [V] # a list with Pytorch tensor V inside to later use .append()
	new_E = []

	with torch.no_grad():
		for edge_ij in E:
			i, j = int(edge_ij[0]), int(edge_ij[1])

			# only create rectangles for edges of vertices in indicated range
			if not (idx_start <= i <= idx_end and idx_start <= j <= idx_end):
				continue
			
			# calculate the edge vector representation and length
			vi = V[i]
			vj = V[j]
			e = vj - vi
			L = e.norm()
			
			# right-hand normal of length c
			n = torch.tensor([e[1], -e[0]]) * (c / L)

			# unsqueeze so [x,y] become [[x,y]]
			vi_shifted = (vi + n).unsqueeze(0)
			vj_shifted = (vj + n).unsqueeze(0)

			# add vertices to new vertex list
			new_V.append(vi_shifted)
			vi_shifted_idx = next_idx;  next_idx += 1
			new_V.append(vj_shifted)
			vj_shifted_idx = next_idx;  next_idx += 1

			# add three new edges, set its type as long instead of integer for future indexing
			new_E += [torch.tensor([j, vj_shifted_idx], dtype=torch.long), torch.tensor([vj_shifted_idx, vi_shifted_idx], dtype=torch.long), torch.tensor([vi_shifted_idx, i], dtype=torch.long)]
	
	all_V = torch.cat(new_V, dim=0)
	all_E = torch.stack(new_E, dim=0) if new_E else torch.empty((0,2),dtype=torch.long)

	return all_V, all_E