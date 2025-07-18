def gradient_descent(f, x0, b0, c0, lr, tol = 1e-6, max_iters = 1000):
	x = x0.clone().detach().requires_grad_(True) 
	b = b0.clone().detach().requires_grad_(True)
	c = c0.clone().detach().requires_grad_(True)

	loss = None

	for i in range(0, max_iters):
		# reset all gradients from previous iterations to 0 
		for param in (x, b, c):
			if param.grad is not None:
				param.grad.zero_()

		loss = f(x, b, c) # treated as a 0-D torch tensor

		# check stopping condition
		if loss.item() < tol:
			break

		# backpropagate and update the param grads with the results
		loss.backward()

		# do the update step in torch.no_grad() so it is not counted in future backpropagation
		with torch.no_grad():
			x -= lr * x.grad 
			b -= lr * b.grad 
			c -= lr * c.grad

	return x, b, c
