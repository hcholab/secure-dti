import datetime
import random
import sys
from string import ascii_lowercase

import matplotlib.pyplot as plt
import numpy as np
from sklearn import metrics

N_HIDDEN = 1
LOSS = 'hinge'


def report_scores(X, y, W, b, act, data_dir):
    y_true = []
    y_pred = []
    y_score = []

    for l in range(N_HIDDEN):
        if l == 0:
            act[l] = np.maximum(0, np.dot(X, W[l]) + b[l])
        else:
            act[l] = np.maximum(0, np.dot(act[l - 1], W[l]) + b[l])

    if N_HIDDEN == 0:
        scores = np.dot(X, W[-1]) + b[-1]
    else:
        scores = np.dot(act[-1], W[-1]) + b[-1]

    predicted_class = np.zeros(scores.shape)
    if LOSS == 'hinge':
        predicted_class[scores > 0] = 1
        predicted_class[scores <= 0] = -1
        y = 2 * y - 1
    else:
        predicted_class[scores >= 0.5] = 1

    sys.stdout.write(str(datetime.datetime.now()) + ' | ')
    print('Batch accuracy: {}'.format(metrics.accuracy_score(y, predicted_class)))

    y_true.extend(list(y))
    y_pred.extend(list(predicted_class))
    y_score.extend(list(scores))

    # Output aggregated scores.
    try:
        sys.stdout.write(str(datetime.datetime.now()) + ' | ')
        print('Accuracy: {0:.2f}'.format(metrics.accuracy_score(y_true, y_pred)))
        sys.stdout.write(str(datetime.datetime.now()) + ' | ')
        print('F1: {0:.2f}'.format(metrics.f1_score(y_true, y_pred)))
        sys.stdout.write(str(datetime.datetime.now()) + ' | ')
        print('Precision: {0:.2f}'.format(metrics.precision_score(y_true, y_pred)))
        sys.stdout.write(str(datetime.datetime.now()) + ' | ')
        print('Recall: {0:.2f}'.format(metrics.recall_score(y_true, y_pred)))
        sys.stdout.write(str(datetime.datetime.now()) + ' | ')
        print('ROC AUC: {0:.2f}'.format(metrics.roc_auc_score(y_true, y_score)))
        sys.stdout.write(str(datetime.datetime.now()) + ' | ')
        print(
            'Avg. precision: {0:.2f}'.format(
                metrics.average_precision_score(y_true, y_score)
            )
        )

        # Plot ROC curve and precision-recall curve in subplots and save to file
        plt.figure(figsize=(12, 5))

        fpr, tpr, _ = metrics.roc_curve(y_true, y_score)
        plt.subplot(1, 2, 1)
        plt.plot(fpr, tpr)
        plt.xlabel('False Positive Rate')
        plt.ylabel('True Positive Rate')
        plt.title(f'ROC Curve')

        plt.subplot(1, 2, 2)
        precision, recall, _ = metrics.precision_recall_curve(y_true, y_score)
        plt.plot(recall, precision)
        plt.xlabel('Recall')
        plt.ylabel('Precision')
        plt.title(f'Precision-Recall Curve')

        plt.tight_layout()
        plt.savefig(f'{data_dir}/roc_pr.png')
    except Exception as e:
        sys.stderr.write(str(e))
        sys.stderr.write('\n')


def load_model(pid: int):
    W = [[] for _ in range(N_HIDDEN + 1)]
    for l in range(N_HIDDEN + 1):
        W[l] = np.loadtxt(f'mpc/cache/test_P{pid}_W{l}_final.bin')

    # Initialize bias vector with zeros.
    b = [[] for _ in range(N_HIDDEN + 1)]
    for l in range(N_HIDDEN + 1):
        b[l] = np.loadtxt(f'mpc/cache/test_P{pid}_b{l}_final.bin')

    # Initialize activations.
    act = [[] for _ in range(N_HIDDEN)]

    return W, b, act


if __name__ == '__main__':
    data_dir = sys.argv[1].rstrip('/')
    pid = int(sys.argv[2])

    W, b, act = load_model(pid)

    X = np.genfromtxt(f'{data_dir}/Xtest', delimiter=1, dtype='float')
    y = np.genfromtxt(f'{data_dir}/ytest', delimiter=1, dtype='float')

    print(f'Testing accuracy:')
    report_scores(X, y, W, b, act, data_dir)
