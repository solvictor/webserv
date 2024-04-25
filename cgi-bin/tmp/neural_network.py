import numpy as np
from sys import stderr
from typing import List
from scipy.special import expit

perr = lambda x, *args, **kwargs: print(x, *args, **kwargs, file=stderr)
BIAS = 1

class Network:
    def __init__(self, n_inputs: int, n_hidden: int, n_outputs: int, neuron_per_hidden: List[int]) -> None:
        self.layers = []

        for layer in range(n_hidden):
            n = n_inputs if layer == 0 else neuron_per_hidden[layer - 1]
            # Use He initialization for ReLU activation
            hidden_layer = [{"weights": np.random.normal(0.0, np.sqrt(2 / n), (n + 1))} for _ in range(neuron_per_hidden[layer])]
            self.layers.append(hidden_layer)

        n = n_inputs if n_hidden == 0 else neuron_per_hidden[n_hidden - 1]
        # Use Xavier/Glorot initialization for the output layer
        output_layer = [{"weights": np.random.normal(0.0, np.sqrt(1 / n), (n + 1))} for _ in range(n_outputs)]
        self.layers.append(output_layer)

    def relu(self, x):
        return np.maximum(0, x)

    def relu_derivative(self, x):
        return np.where(x > 0, 1, 0)

    def sigmoid(self, x):
        return expit(x)

    def sigmoid_derivative(self, x):
        return x * (1 - x)

    def forward_propagate(self, inputs: List[int]) -> List[int]:
        for layer_index, layer in enumerate(self.layers):
            new_inputs = []
            for neuron in layer:
                activation = np.dot(inputs + [BIAS], neuron['weights'])
                
                # Use ReLU activation for hidden layers
                if layer_index < len(self.layers) - 1:
                    neuron['output'] = self.relu(activation)
                else:
                    # Use sigmoid activation for the output layer
                    neuron['output'] = self.sigmoid(activation)
                
                new_inputs.append(neuron['output'])
            inputs = new_inputs
        return inputs

    def backward_propagate(self, expected: List[int]) -> None:
        for i in reversed(range(len(self.layers))):
            for j, neuron in enumerate(self.layers[i]):
                output = neuron['output']
                
                # Use ReLU derivative for hidden layers and sigmoid derivative for the output layer
                if i != len(self.layers) - 1:
                    neuron['delta'] = self.relu_derivative(output) * np.sum(
                        [n['delta'] * n['weights'][j] for n in self.layers[i + 1]]
                    )
                else:
                    neuron['delta'] = self.sigmoid_derivative(output) * (output - expected[j])

    def update_weights(self, l_rate, weight_decay, _input):
        _input = np.array(_input)
        for layer in range(len(self.layers)):
            for neuron in self.layers[layer]:
                neuron['weights'][:-1] += -l_rate * (neuron['delta'] + weight_decay * neuron['weights'][:-1]) * (
                    _input if layer == 0 else np.array([n['output'] for n in self.layers[layer - 1]])
                )
                neuron['weights'][-1] += -l_rate * neuron['delta']

    def train(self, train, l_rate, n_epoch, weight_decay=0.001):
        for epoch in range(n_epoch):
            sum_error = 0
            for _input, expected in train:
                outputs = self.forward_propagate(_input)
                sum_error += np.sum((np.array(expected) - np.array(outputs)) ** 2)
                self.backward_propagate(expected)
                self.update_weights(l_rate, weight_decay, _input)
            perr(f">epoch={str(epoch).zfill(len(str(n_epoch)))}, error={sum_error:.3f}")
            if sum_error == 0:
                break

    def predict(self, data: List[int]) -> List[int]:
        outputs = self.forward_propagate(data)
        return [*map(round, outputs)]

tests, training_sets = map(int, input().split())
test_data = [[*map(int, input())] for _ in range(tests)]
training_data = []

for _ in range(training_sets):
    training_inputs, expected_outputs = input().split()
    training_data += [([*map(int, training_inputs)], [*map(int, expected_outputs)])]

learning_rate = 0.1
hidden_layers = 4
neuron_per_hidden = [32] * hidden_layers
training_iterations = 1000
network = Network(32, hidden_layers, 32, neuron_per_hidden)
network.train(training_data, learning_rate, training_iterations)
for data in test_data:
    print(*network.predict(data), sep='')