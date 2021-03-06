//
//  momentumUpdater.h
//  LightCTR
//
//  Created by SongKuangshi on 2017/11/15.
//  Copyright © 2017年 SongKuangshi. All rights reserved.
//

#ifndef momentumUpdater_h
#define momentumUpdater_h

#include "gradientUpdater.h"

class MomentumUpdater : public GradientUpdater {
public:
    static double __global_momentum;
};

class AdadeltaUpdater : public MomentumUpdater {
public:
    void learnable_params_cnt(size_t cnt) {
        __adadelta_params_cnt = cnt;
        __adadelta_acc_size = cnt * 2;
        __adadelta_accum.resize(__adadelta_acc_size);
        tmp = tmp_E = NULL;
    }
    void clear() {
        for (size_t i = 0; i < __adadelta_acc_size; i++) {
            __adadelta_accum[i]->zeroInit();
        }
    }
    void update(size_t offset, vector<Matrix*>& weight, vector<Matrix*>& grad) {
        assert(weight.size() == grad.size());
        assert(offset + weight.size() <= __adadelta_params_cnt);
        for (size_t i = 0; i < weight.size(); i++) {
            if (tmp) {
                tmp->reshape(grad[i]->x_len, grad[i]->y_len);
                tmp_E->reshape(grad[i]->x_len, grad[i]->y_len);
            }
            tmp = grad[i]->scale(1.0 / __global_minibatch_size)->copy(tmp);
            tmp->pow(2.0);
            
            if (__adadelta_accum[offset + i] == NULL) {
                __adadelta_accum[offset + i] = new Matrix(tmp->x_len, tmp->y_len);
                __adadelta_accum[offset + i]->zeroInit();
            }
            __adadelta_accum[offset + i]->add(tmp, 1.0 - __global_momentum, __global_momentum);
            
            tmp = __adadelta_accum[__adadelta_params_cnt + offset + i]->copy(tmp);
            tmp->add(1e-12);
            tmp_E = __adadelta_accum[offset + i]->copy(tmp_E);
            tmp_E->add(1e-12);
            
            tmp->dotProduct(tmp_E->inverse())->pow(0.5);
            grad[i]->dotProduct(tmp);
            
            grad[i]->copy(tmp)->pow(2);
            __adadelta_accum[__adadelta_params_cnt + offset + i]->add(tmp, 1.0 - __global_momentum, __global_momentum);
            
            weight[i]->subtract(grad[i]);
            grad[i]->zeroInit();
        }
    }
private:
    Matrix *tmp, *tmp_E;
    vector<Matrix*> __adadelta_accum;
    size_t __adadelta_params_cnt, __adadelta_acc_size;
};

class AdadeltaUpdater_Num : public MomentumUpdater {
public:
    void learnable_params_cnt(size_t cnt) {
        __adadelta_params_cnt = cnt;
        __adadelta_acc_size = cnt * 2;
        __adadelta_accum.resize(__adadelta_acc_size);
        fill(__adadelta_accum.begin(), __adadelta_accum.end(), 0.0f);
    }
    void clear() {
        for (size_t i = 0; i < __adadelta_acc_size; i++) {
            __adadelta_accum[i] = 0;
        }
    }
    template<typename T>
    void update(size_t offset, size_t len, T& weight, T& grad) {
        assert(offset + len <= __adadelta_params_cnt);
        for (size_t i = 0; i < len; i++) {
            double g = grad[i] / __global_minibatch_size, tmp;
            if (g != 0) {
                __adadelta_accum[offset + i] = __adadelta_accum[offset + i] * __global_momentum
                                               + (1.0 - __global_momentum) * g * g;
                tmp = (__adadelta_accum[__adadelta_params_cnt + offset + i] + 1e-12)
                      / (__adadelta_accum[offset + i] + 1e-12);
                g *= sqrt(tmp);
                
                __adadelta_accum[__adadelta_params_cnt + offset + i] =
                    __adadelta_accum[__adadelta_params_cnt + offset + i] * __global_momentum
                    + (1.0 - __global_momentum) * g * g;
                
                weight[i] -= g;
            }
            grad[i] = 0.0f;
        }
    }
private:
    vector<double> __adadelta_accum;
    size_t __adadelta_params_cnt, __adadelta_acc_size;
};

#endif /* momentumUpdater_h */
