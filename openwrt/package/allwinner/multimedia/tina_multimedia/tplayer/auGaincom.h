#ifndef	___aumixcommon__
#define	___aumixcommon__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef	struct ___AudioGain__{
    //input para
    int	    preamp;//-20 -- 20 db//max32db
    int   InputChan;
    short *InputPtr;
    int		InputLen;//total byte
    int   OutputChan;//0 ����� 1: ���������only 2�������������  3: double left 4:double right
    short *OutputPtr;
}AudioGain;
int	tina_do_AudioGain(AudioGain	*AGX);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
