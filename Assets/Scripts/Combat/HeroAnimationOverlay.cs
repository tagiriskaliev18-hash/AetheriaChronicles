using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Procedural touches layered on top of the Animator in LateUpdate (after it has written the pose):
    /// guard stance, tuck while airborne or dashing, body lean into acceleration and a head that follows
    /// the camera. Every rotation is applied once per frame on top of the animated pose, so nothing drifts.
    /// </summary>
    public class HeroAnimationOverlay : MonoBehaviour
    {
        public float BlockWeight;      // 0..1 guard stance
        public float TuckWeight;       // 0..1 limbs tucked (jump, dash)
        public float LeanPitch;        // degrees, + = leaning forward
        public float LeanRoll;         // degrees, + = leaning right
        public float LookWeight;       // 0..1 head tracking strength
        public Vector3 LookTarget;

        public Transform VisualRoot;

        private Transform _upperL, _lowerL, _upperR, _lowerR, _handL, _handR, _spine, _head, _thighL, _thighR, _calfL, _calfR;
        private bool _bound;

        public void Bind(GameObject model)
        {
            _bound = false;
            if (model == null) return;
            _upperL = Find(model.transform, "upperarm_l");
            _lowerL = Find(model.transform, "lowerarm_l");
            _upperR = Find(model.transform, "upperarm_r");
            _lowerR = Find(model.transform, "lowerarm_r");
            _handL = Find(model.transform, "hand_l");
            _handR = Find(model.transform, "hand_r");
            _spine = Find(model.transform, "spine_02") ?? Find(model.transform, "spine_01");
            _head = Find(model.transform, "head");
            _thighL = Find(model.transform, "thigh_l");
            _thighR = Find(model.transform, "thigh_r");
            _calfL = Find(model.transform, "calf_l");
            _calfR = Find(model.transform, "calf_r");
            _bound = _upperL != null || _upperR != null;
        }

        private static Transform Find(Transform root, params string[] names) => AetheriaPalette.FindBoneAny(root, names);

        private void LateUpdate()
        {
            // Body lean of the whole visual: pitch around the chest height so the feet stay planted
            if (VisualRoot != null && (Mathf.Abs(LeanPitch) > 0.01f || Mathf.Abs(LeanRoll) > 0.01f))
            {
                Quaternion rot = Quaternion.Euler(LeanPitch, 0f, LeanRoll);
                Vector3 pivot = new Vector3(0f, 1.05f, 0f);
                VisualRoot.localRotation = rot;
                VisualRoot.localPosition = pivot - rot * pivot;
            }
            else if (VisualRoot != null)
            {
                VisualRoot.localRotation = Quaternion.identity;
                VisualRoot.localPosition = Vector3.zero;
            }

            if (!_bound) return;

            Vector3 fwd = transform.forward, up = Vector3.up, right = transform.right;

            if (BlockWeight > 0.01f)
            {
                GuardSide(_upperL, _lowerL, _handL, (fwd * 0.80f - right * 0.42f + up * 0.12f), fwd * 0.92f + up * 0.18f - right * 0.10f);
                GuardSide(_upperR, _lowerR, _handR, (fwd * 0.80f + right * 0.42f + up * 0.12f), fwd * 0.92f + up * 0.18f + right * 0.10f);
                if (_spine != null) _spine.rotation = Quaternion.AngleAxis(7f * BlockWeight, right) * _spine.rotation;
            }

            if (TuckWeight > 0.01f)
            {
                float t = TuckWeight;
                if (_thighL != null) _thighL.rotation = Quaternion.AngleAxis(46f * t, right) * _thighL.rotation;
                if (_thighR != null) _thighR.rotation = Quaternion.AngleAxis(52f * t, right) * _thighR.rotation;
                if (_calfL != null) _calfL.rotation = Quaternion.AngleAxis(-62f * t, right) * _calfL.rotation;
                if (_calfR != null) _calfR.rotation = Quaternion.AngleAxis(-70f * t, right) * _calfR.rotation;
                if (_upperL != null) _upperL.rotation = Quaternion.AngleAxis(-38f * t, right) * _upperL.rotation;
                if (_upperR != null) _upperR.rotation = Quaternion.AngleAxis(-38f * t, right) * _upperR.rotation;
            }

            if (LookWeight > 0.01f && _head != null)
            {
                Vector3 dir = LookTarget - _head.position;
                if (dir.sqrMagnitude > 0.01f)
                {
                    Quaternion wanted = Quaternion.RotateTowards(_head.rotation, Quaternion.LookRotation(dir, up), 52f);
                    _head.rotation = Quaternion.Slerp(_head.rotation, wanted, LookWeight);
                }
            }
        }

        /// <summary>Brings one arm into a guard: upper arm toward <paramref name="elbowDir"/>, forearm toward <paramref name="handDir"/>.</summary>
        private void GuardSide(Transform upper, Transform lower, Transform hand, Vector3 elbowDir, Vector3 handDir)
        {
            float w = Mathf.Clamp01(BlockWeight);
            if (upper != null && lower != null)
            {
                Vector3 cur = lower.position - upper.position;
                if (cur.sqrMagnitude > 0.0001f)
                {
                    Quaternion delta = Quaternion.FromToRotation(cur.normalized, Vector3.Slerp(cur.normalized, elbowDir.normalized, w));
                    upper.rotation = Quaternion.Slerp(Quaternion.identity, delta, w) * upper.rotation;
                }
            }
            if (lower != null && hand != null)
            {
                Vector3 cur2 = hand.position - lower.position;
                if (cur2.sqrMagnitude > 0.0001f)
                {
                    Quaternion delta2 = Quaternion.FromToRotation(cur2.normalized, Vector3.Slerp(cur2.normalized, handDir.normalized, w));
                    lower.rotation = Quaternion.Slerp(Quaternion.identity, delta2, w) * lower.rotation;
                }
            }
        }
    }
}
