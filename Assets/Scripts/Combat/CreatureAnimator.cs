using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Talks to a creature's Animator using one shared vocabulary, whatever the creature is.
    /// Every creature controller built for the game has: float Speed, int Action + trigger DoAction
    /// (1..4 attacks, 5 cast, 6 stun), triggers Hit / Die / Revive, and states named Idle, Move,
    /// Attack1..Attack4, Cast, Stun, Hit, Death. Controllers that lack a clip use a stand-in, so
    /// gameplay code never has to know which creature it is driving.
    /// </summary>
    public class CreatureAnimator : MonoBehaviour
    {
        public enum Kind { Boss, Whisper, Minion, Brute, Dragon }

        public Kind CreatureKind = Kind.Minion;
        public float MoveAnimationSpeed = 3.6f;     // speed the Move clip is authored for
        public float AttackTimeScale = 1.0f;
        public bool HasMoveClip = true;

        public Animator Animator { get; private set; }
        public bool Valid => Animator != null && Animator.isActiveAndEnabled && Animator.runtimeAnimatorController != null;
        public bool InAttack { get; private set; }
        public bool InStun { get; private set; }
        public bool Dead { get; private set; }
        /// <summary>Normalized time (seconds) spent in the current action.</summary>
        public float ActionTime { get; private set; }
        public int CurrentAction { get; private set; }

        private static readonly int PSpeed = Animator.StringToHash("Speed");
        private static readonly int PAction = Animator.StringToHash("Action");
        private static readonly int TDo = Animator.StringToHash("DoAction");
        private static readonly int THit = Animator.StringToHash("Hit");
        private static readonly int TDie = Animator.StringToHash("Die");
        private static readonly int TRevive = Animator.StringToHash("Revive");
        private static readonly int SAttack1 = Animator.StringToHash("Attack1");
        private static readonly int SAttack2 = Animator.StringToHash("Attack2");
        private static readonly int SAttack3 = Animator.StringToHash("Attack3");
        private static readonly int SAttack4 = Animator.StringToHash("Attack4");
        private static readonly int SCast = Animator.StringToHash("Cast");
        private static readonly int SStun = Animator.StringToHash("Stun");
        private static readonly int SDeath = Animator.StringToHash("Death");

        public void Bind(Animator animator)
        {
            Animator = animator;
            if (Animator == null) return;
            if (Animator.runtimeAnimatorController == null || Animator.avatar == null || !Animator.avatar.isValid)
            {
                Animator = null;
                return;
            }
            Animator.applyRootMotion = false;
            Animator.cullingMode = AnimatorCullingMode.AlwaysAnimate;
        }

        /// <summary>True while the base layer is in the given action's state.</summary>
        public bool InActionState(int action, out float normalizedTime)
        {
            normalizedTime = 0f;
            if (!Valid) return false;
            var st = Animator.GetCurrentAnimatorStateInfo(0);
            normalizedTime = st.normalizedTime % 1f;
            if (st.normalizedTime >= 1f) normalizedTime = st.normalizedTime;
            switch (action)
            {
                case 1: return st.shortNameHash == SAttack1;
                case 2: return st.shortNameHash == SAttack2;
                case 3: return st.shortNameHash == SAttack3;
                case 4: return st.shortNameHash == SAttack4;
                case 5: return st.shortNameHash == SCast;
                case 6: return st.shortNameHash == SStun;
                default: return false;
            }
        }

        public bool InDeathState => Valid && Animator.GetCurrentAnimatorStateInfo(0).shortNameHash == SDeath;

        public void Drive(float planarSpeed)
        {
            if (!Valid) return;
            float s = HasMoveClip ? planarSpeed / Mathf.Max(0.5f, MoveAnimationSpeed) : 0f;
            Animator.SetFloat(PSpeed, Mathf.Clamp01(s));
        }

        /// <summary>Starts an action (1..4 attack, 5 cast, 6 stun). Returns false if the creature cannot animate.</summary>
        public bool PlayAction(int action)
        {
            if (!Valid || Dead) return false;
            Animator.SetInteger(PAction, action);
            Animator.SetTrigger(TDo);
            CurrentAction = action;
            ActionTime = 0f;
            InAttack = action >= 1 && action <= 4;
            InStun = action == 6;
            return true;
        }

        public void PlayHit()
        {
            if (!Valid || Dead) return;
            Animator.SetTrigger(THit);
        }

        public void PlayDeath()
        {
            Dead = true;
            InAttack = false;
            InStun = false;
            if (!Valid) return;
            Animator.SetTrigger(TDie);
        }

        public void Reset()
        {
            Dead = false;
            InAttack = false;
            InStun = false;
            ActionTime = 0f;
            if (!Valid) return;
            Animator.Rebind();
            Animator.Update(0f);
            Animator.SetTrigger(TRevive);
        }

        private void Update()
        {
            if (InAttack || InStun) ActionTime += Time.deltaTime;
            if ((InAttack || InStun) && Valid && !InActionState(CurrentAction, out _))
            {
                // The action's state has been left (interrupted or finished)
                var st = Animator.GetCurrentAnimatorStateInfo(0);
                if (st.shortNameHash == SAttack1 || st.shortNameHash == SAttack2 || st.shortNameHash == SAttack3 ||
                    st.shortNameHash == SAttack4 || st.shortNameHash == SStun || st.shortNameHash == SCast)
                    return;
                InAttack = false;
                InStun = false;
            }
        }
    }
}
